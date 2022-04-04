// Copyright 2015-2021 Tier IV, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <boost/range/adaptor/sliced.hpp>
#include <concealer/autoware_universe.hpp>

namespace concealer
{
AutowareUniverse::~AutowareUniverse() { shutdownAutoware(); }

auto AutowareUniverse::initialize(const geometry_msgs::msg::Pose & initial_pose) -> void
{
  if (not std::exchange(initialize_was_called, true)) {
    task_queue.delay([this, initial_pose]() {
      set(initial_pose);
      waitForAutowareStateToBeInitializing();
      waitForAutowareStateToBeWaitingForRoute([&]() {
        InitialPose initial_pose;
        {
          initial_pose.header.stamp = get_clock()->now();
          initial_pose.header.frame_id = "map";
          initial_pose.pose.pose = current_pose;
        }
        return setInitialPose(initial_pose);
      });

      // TODO(yamacir-kit) AFTER /api/autoware/set/initialize_pose IS SUPPORTED.
      // waitForAutowareStateToBeWaitingForRoute([&]() {
      //   auto request = std::make_shared<InitializePose::Request>();
      //   request->pose.header.stamp = get_clock()->now();
      //   request->pose.header.frame_id = "map";
      //   request->pose.pose.pose = initial_pose;
      //   requestInitializePose(request);
      // });
    });
  }
}

auto AutowareUniverse::plan(const std::vector<geometry_msgs::msg::PoseStamped> & route) -> void
{
  assert(not route.empty());
  sleep(10);
  task_queue.delay([this, route] {
    waitForAutowareStateToBeWaitingForRoute();  // NOTE: This is assertion.
    setGoalPose(route.back());
    for (const auto & each : route | boost::adaptors::sliced(0, route.size() - 1)) {
      setCheckpoint(each);
    }
    waitForAutowareStateToBePlanning();
    waitForAutowareStateToBeWaitingForEngage();  // NOTE: Autoware.IV 0.11.1 waits about 3 sec from the completion of Planning until the transition to WaitingForEngage.
  });
}

auto AutowareUniverse::engage() -> void
{
  task_queue.delay([this]() {
    waitForAutowareStateToBeDriving([this]() {
      auto request = std::make_shared<Engage::Request>();
      request->engage = true;
      requestEngage(request);
    });
  });
}

auto AutowareUniverse::update() -> void
{
  CurrentControlMode current_control_mode;
  {
    current_control_mode.mode = CurrentControlMode::AUTONOMOUS;
  }
  setCurrentControlMode(current_control_mode);

  CurrentShift current_shift;
  {
    using autoware_auto_vehicle_msgs::msg::GearReport;
    current_shift.stamp = get_clock()->now();
    current_shift.report = current_twist.linear.x >= 0 ? GearReport::DRIVE : GearReport::REVERSE;
  }
  setCurrentShift(current_shift);

  CurrentSteering current_steering;
  {
    current_steering.stamp = get_clock()->now();
    current_steering.steering_tire_angle = getSteeringAngle();
  }
  setCurrentSteering(current_steering);

  CurrentVelocity current_velocity;
  {
    current_velocity.header.stamp = get_clock()->now();
    current_velocity.header.frame_id = "base_link";
    current_velocity.longitudinal_velocity = current_twist.linear.x;
    current_velocity.lateral_velocity = current_twist.linear.y;
    current_velocity.heading_rate = current_twist.angular.z;
  }
  setCurrentVelocity(current_velocity);

  LocalizationOdometry localization_odometry;
  {
    localization_odometry.header.stamp = get_clock()->now();
    localization_odometry.header.frame_id = "map";
    localization_odometry.pose.pose = current_pose;
    localization_odometry.pose.covariance = {};
    localization_odometry.twist.twist = current_twist;
  }
  setLocalizationOdometry(localization_odometry);

  setTransform(current_pose);
}

auto AutowareUniverse::getAcceleration() const -> double
{
  return getAckermannControlCommand().longitudinal.acceleration;
}

auto AutowareUniverse::getVelocity() const -> double
{
  return getAckermannControlCommand().longitudinal.speed;
}

auto AutowareUniverse::getSteeringAngle() const -> double
{
  return getAckermannControlCommand().lateral.steering_tire_angle;
}

auto AutowareUniverse::getGearSign() const -> double
{
  using autoware_auto_vehicle_msgs::msg::GearCommand;
  return getGearCommand().command == GearCommand::REVERSE ? -1.0 : 1.0;
}

auto AutowareUniverse::getWaypoints() const -> traffic_simulator_msgs::msg::WaypointsArray
{
  traffic_simulator_msgs::msg::WaypointsArray waypoints;

  for (const auto & point : getTrajectory().points) {
    waypoints.waypoints.emplace_back(point.pose.position);
  }

  return waypoints;
}

auto AutowareUniverse::restrictTargetSpeed(double value) const -> double
{
  // no restrictions here
  return value;
}

auto AutowareUniverse::getAutowareStateString() const -> std::string
{
  using autoware_auto_system_msgs::msg::AutowareState;

#define CASE(IDENTIFIER)          \
  case AutowareState::IDENTIFIER: \
    return #IDENTIFIER

  switch (getAutowareState().state) {
    CASE(INITIALIZING);
    CASE(WAITING_FOR_ROUTE);
    CASE(PLANNING);
    CASE(WAITING_FOR_ENGAGE);
    CASE(DRIVING);
    CASE(ARRIVED_GOAL);
    CASE(FINALIZING);

    default:
      return "";
  }

#undef CASE
}

auto AutowareUniverse::sendSIGINT() -> void  //
{
  ::kill(process_id, SIGINT);
}

auto AutowareUniverse::setVelocityLimit(double velocity_limit) -> void
{
  auto request = std::make_shared<SetVelocityLimit::Request>();
  request->velocity = velocity_limit;
  requestSetVelocityLimit(request);
}

auto AutowareUniverse::isReady() noexcept -> bool
{
  return is_ready or (is_ready = isWaitingForRoute());
}

auto AutowareUniverse::getVehicleCommand() const -> std::tuple<
  autoware_auto_control_msgs::msg::AckermannControlCommand,
  autoware_auto_vehicle_msgs::msg::GearCommand>
{
  return std::make_tuple(getAckermannControlCommand(), getGearCommand());
}
}  // namespace concealer

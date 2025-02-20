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

#ifndef CONCEALER__AUTOWARE_UNIVERSE_HPP_
#define CONCEALER__AUTOWARE_UNIVERSE_HPP_

#include <autoware_auto_control_msgs/msg/ackermann_control_command.hpp>
#include <autoware_auto_perception_msgs/msg/traffic_signal_array.hpp>
#include <autoware_auto_planning_msgs/msg/had_map_route.hpp>
#include <autoware_auto_planning_msgs/msg/path_with_lane_id.hpp>
#include <autoware_auto_planning_msgs/msg/trajectory.hpp>
#include <autoware_auto_system_msgs/msg/autoware_state.hpp>
#include <autoware_auto_vehicle_msgs/msg/control_mode_report.hpp>
#include <autoware_auto_vehicle_msgs/msg/gear_command.hpp>
#include <autoware_auto_vehicle_msgs/msg/gear_report.hpp>
#include <autoware_auto_vehicle_msgs/msg/steering_report.hpp>
#include <autoware_auto_vehicle_msgs/msg/turn_indicators_command.hpp>
#include <autoware_auto_vehicle_msgs/msg/turn_indicators_report.hpp>
#include <autoware_auto_vehicle_msgs/msg/velocity_report.hpp>
#include <concealer/autoware.hpp>
#include <concealer/dirty_hack.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tier4_external_api_msgs/srv/engage.hpp>
// TODO #include <tier4_external_api_msgs/srv/initialize_pose.hpp>
#include <tier4_external_api_msgs/srv/set_velocity_limit.hpp>

namespace concealer
{
class AutowareUniverse : public Autoware, public TransitionAssertion<AutowareUniverse>
{
  friend class TransitionAssertion<AutowareUniverse>;

  bool is_ready = false;

  using Checkpoint = geometry_msgs::msg::PoseStamped;
  using CurrentControlMode = autoware_auto_vehicle_msgs::msg::ControlModeReport;
  using CurrentShift = autoware_auto_vehicle_msgs::msg::GearReport;
  using CurrentSteering = autoware_auto_vehicle_msgs::msg::SteeringReport;
  using CurrentTurnIndicators = autoware_auto_vehicle_msgs::msg::TurnIndicatorsReport;
  using CurrentTwist = geometry_msgs::msg::TwistStamped;
  using CurrentVelocity = autoware_auto_vehicle_msgs::msg::VelocityReport;
  using GoalPose = geometry_msgs::msg::PoseStamped;
  using InitialPose = geometry_msgs::msg::PoseWithCovarianceStamped;
  using LocalizationOdometry = nav_msgs::msg::Odometry;

  CONCEALER_DEFINE_PUBLISHER(Checkpoint);
  CONCEALER_DEFINE_PUBLISHER(CurrentControlMode);
  CONCEALER_DEFINE_PUBLISHER(CurrentShift);
  CONCEALER_DEFINE_PUBLISHER(CurrentSteering);
  CONCEALER_DEFINE_PUBLISHER(CurrentTurnIndicators);
  CONCEALER_DEFINE_PUBLISHER(CurrentTwist);
  CONCEALER_DEFINE_PUBLISHER(CurrentVelocity);
  CONCEALER_DEFINE_PUBLISHER(GoalPose);
  CONCEALER_DEFINE_PUBLISHER(InitialPose);
  CONCEALER_DEFINE_PUBLISHER(LocalizationOdometry);

  using AckermannControlCommand = autoware_auto_control_msgs::msg::AckermannControlCommand;
  using AutowareState = autoware_auto_system_msgs::msg::AutowareState;
  using GearCommand = autoware_auto_vehicle_msgs::msg::GearCommand;
  using PathWithLaneId = autoware_auto_planning_msgs::msg::PathWithLaneId;
  using Trajectory = autoware_auto_planning_msgs::msg::Trajectory;
  using TurnIndicatorsCommand = autoware_auto_vehicle_msgs::msg::TurnIndicatorsCommand;

  CONCEALER_DEFINE_SUBSCRIPTION(AckermannControlCommand);
  CONCEALER_DEFINE_SUBSCRIPTION(AutowareState);
  CONCEALER_DEFINE_SUBSCRIPTION(GearCommand);
  CONCEALER_DEFINE_SUBSCRIPTION(PathWithLaneId);
  CONCEALER_DEFINE_SUBSCRIPTION(Trajectory);
  CONCEALER_DEFINE_SUBSCRIPTION(TurnIndicatorsCommand);

  using Engage = tier4_external_api_msgs::srv::Engage;
  // TODO using InitializePose = tier4_external_api_msgs::srv::InitializePose;
  using SetVelocityLimit = tier4_external_api_msgs::srv::SetVelocityLimit;

  CONCEALER_DEFINE_CLIENT(Engage);
  // TODO CONCEALER_DEFINE_CLIENT(InitializePose);
  CONCEALER_DEFINE_CLIENT(SetVelocityLimit);

public:
#define DEFINE_STATE_PREDICATE(NAME, VALUE)                              \
  auto is##NAME() const noexcept                                         \
  {                                                                      \
    using autoware_auto_system_msgs::msg::AutowareState;                 \
    return current_value_of_AutowareState.state == AutowareState::VALUE; \
  }                                                                      \
  static_assert(true, "")

  DEFINE_STATE_PREDICATE(Initializing, INITIALIZING);            // 1
  DEFINE_STATE_PREDICATE(WaitingForRoute, WAITING_FOR_ROUTE);    // 2
  DEFINE_STATE_PREDICATE(Planning, PLANNING);                    // 3
  DEFINE_STATE_PREDICATE(WaitingForEngage, WAITING_FOR_ENGAGE);  // 4
  DEFINE_STATE_PREDICATE(Driving, DRIVING);                      // 5
  DEFINE_STATE_PREDICATE(ArrivedGoal, ARRIVED_GOAL);             // 6
  DEFINE_STATE_PREDICATE(Finalizing, FINALIZING);                // 7

#undef DEFINE_STATE_PREDICATE

  template <typename... Ts>
  CONCEALER_PUBLIC explicit AutowareUniverse(Ts &&... xs)
  : Autoware(std::forward<decltype(xs)>(xs)...),
    // clang-format off
    CONCEALER_INIT_PUBLISHER(Checkpoint, "/planning/mission_planning/checkpoint"),
    CONCEALER_INIT_PUBLISHER(CurrentControlMode, "/vehicle/status/control_mode"),
    CONCEALER_INIT_PUBLISHER(CurrentShift, "/vehicle/status/gear_status"),
    CONCEALER_INIT_PUBLISHER(CurrentSteering, "/vehicle/status/steering_status"),
    CONCEALER_INIT_PUBLISHER(CurrentTurnIndicators, "/vehicle/status/turn_indicators_status"),
    CONCEALER_INIT_PUBLISHER(CurrentVelocity, "/vehicle/status/velocity_status"),
    CONCEALER_INIT_PUBLISHER(GoalPose, "/planning/mission_planning/goal"),
    CONCEALER_INIT_PUBLISHER(InitialPose, "/initialpose"),
    CONCEALER_INIT_PUBLISHER(LocalizationOdometry, "/localization/kinematic_state"),
    CONCEALER_INIT_SUBSCRIPTION(AckermannControlCommand, "/control/command/control_cmd"),
    CONCEALER_INIT_SUBSCRIPTION(AutowareState, "/autoware/state"),
    CONCEALER_INIT_SUBSCRIPTION(GearCommand, "/control/command/gear_cmd"),
    CONCEALER_INIT_SUBSCRIPTION(PathWithLaneId, "/planning/scenario_planning/lane_driving/behavior_planning/path_with_lane_id"),
    CONCEALER_INIT_SUBSCRIPTION(Trajectory, "/planning/scenario_planning/trajectory"),
    CONCEALER_INIT_SUBSCRIPTION(TurnIndicatorsCommand, "/control/command/turn_indicators_cmd"),
    CONCEALER_INIT_CLIENT(Engage, "/api/autoware/set/engage"),
    // TODO CONCEALER_INIT_CLIENT(InitializePose, "/api/autoware/set/initialize_pose"),
    CONCEALER_INIT_CLIENT(SetVelocityLimit, "/api/autoware/set/velocity_limit")
  // clang-format on
  {
    waitpid_options = 0;

    resetTimerCallback();
  }

  virtual ~AutowareUniverse();

  auto engage() -> void override;

  auto getAcceleration() const -> double override;

  auto getAutowareStateString() const -> std::string override;

  auto getGearSign() const -> double override;

  auto getSteeringAngle() const -> double override;

  auto getVehicleCommand() const -> std::tuple<
    autoware_auto_control_msgs::msg::AckermannControlCommand,
    autoware_auto_vehicle_msgs::msg::GearCommand> override;

  auto getVelocity() const -> double override;

  auto getWaypoints() const -> traffic_simulator_msgs::msg::WaypointsArray override;

  auto initialize(const geometry_msgs::msg::Pose &) -> void override;

  auto isReady() noexcept -> bool;

  auto plan(const std::vector<geometry_msgs::msg::PoseStamped> &) -> void override;

  auto restrictTargetSpeed(double) const -> double override;

  auto sendSIGINT() -> void override;

  auto setVelocityLimit(double) -> void override;

  auto update() -> void override;
};
}  // namespace concealer

#endif  // CONCEALER__AUTOWARE_UNIVERSE_HPP_

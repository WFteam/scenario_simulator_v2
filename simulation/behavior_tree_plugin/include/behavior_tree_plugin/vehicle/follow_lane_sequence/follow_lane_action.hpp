// Copyright 2015-2020 Tier IV, Inc. All rights reserved.
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

#ifndef BEHAVIOR_TREE_PLUGIN__VEHICLE__FOLLOW_LANE_SEQUENCE__FOLLOW_LANE_ACTION_HPP_
#define BEHAVIOR_TREE_PLUGIN__VEHICLE__FOLLOW_LANE_SEQUENCE__FOLLOW_LANE_ACTION_HPP_

#include <behavior_tree_plugin/vehicle/behavior_tree.hpp>
#include <behavior_tree_plugin/vehicle/vehicle_action_node.hpp>
#include <string>
#include <vector>

namespace entity_behavior
{
namespace vehicle
{
namespace follow_lane_sequence
{
class FollowLaneAction : public entity_behavior::VehicleActionNode
{
public:
  FollowLaneAction(const std::string & name, const BT::NodeConfiguration & config);
  BT::NodeStatus tick() override;
  void getBlackBoardValues();
  static BT::PortsList providedPorts()
  {
    BT::PortsList ports = {};
    BT::PortsList parent_ports = entity_behavior::VehicleActionNode::providedPorts();
    for (const auto & parent_port : parent_ports) {
      ports.emplace(parent_port.first, parent_port.second);
    }
    return ports;
  }
  const traffic_simulator_msgs::msg::WaypointsArray calculateWaypoints() override;
  const boost::optional<traffic_simulator_msgs::msg::Obstacle> calculateObstacle(
    const traffic_simulator_msgs::msg::WaypointsArray & waypoints) override;

private:
  boost::optional<traffic_simulator_msgs::msg::LaneletPose> target_lanelet_pose_;
};
}  // namespace follow_lane_sequence
}  // namespace vehicle
}  // namespace entity_behavior

#endif  // BEHAVIOR_TREE_PLUGIN__VEHICLE__FOLLOW_LANE_SEQUENCE__FOLLOW_LANE_ACTION_HPP_

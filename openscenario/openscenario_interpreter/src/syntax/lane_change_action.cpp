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

#include <openscenario_interpreter/procedure.hpp>
#include <openscenario_interpreter/reader/attribute.hpp>
#include <openscenario_interpreter/reader/element.hpp>
#include <openscenario_interpreter/syntax/absolute_target_lane.hpp>
#include <openscenario_interpreter/syntax/lane_change_action.hpp>
#include <openscenario_interpreter/syntax/relative_target_lane.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
LaneChangeAction::LaneChangeAction(const pugi::xml_node & node, Scope & scope)
: Scope(scope),
  target_lane_offset(readAttribute<Double>("targetLaneOffset", node, local(), Double())),
  lane_change_action_dynamics(
    readElement<TransitionDynamics>("LaneChangeActionDynamics", node, local())),
  lane_change_target(readElement<LaneChangeTarget>("LaneChangeTarget", node, local()))
{
}

auto LaneChangeAction::accomplished() -> bool
{
  return std::all_of(std::begin(accomplishments), std::end(accomplishments), [](auto && each) {
    const auto is_lane_changing = [](auto &&... xs) {
      return getCurrentAction(std::forward<decltype(xs)>(xs)...) == "lane_change";
    };
    return each.second or (each.second = not is_lane_changing(each.first));
  });
}

auto LaneChangeAction::endsImmediately() noexcept -> bool { return false; }

auto LaneChangeAction::run() noexcept -> void {}

auto LaneChangeAction::start() -> void
{
  accomplishments.clear();

  for (const auto & actor : actors) {
    accomplishments.emplace(actor, false);
  }

  for (const auto & accomplishment : accomplishments) {
    if (lane_change_target.is<AbsoluteTargetLane>()) {
      return requestLaneChange(
        accomplishment.first, static_cast<traffic_simulator::lane_change::AbsoluteTarget>(*this),
        static_cast<traffic_simulator::lane_change::TrajectoryShape>(
          lane_change_action_dynamics.dynamics_shape),
        static_cast<traffic_simulator::lane_change::Constraint>(lane_change_action_dynamics));
    } else {
      return requestLaneChange(
        accomplishment.first, static_cast<traffic_simulator::lane_change::RelativeTarget>(*this),
        static_cast<traffic_simulator::lane_change::TrajectoryShape>(
          lane_change_action_dynamics.dynamics_shape),
        static_cast<traffic_simulator::lane_change::Constraint>(lane_change_action_dynamics));
    }
  }
}

LaneChangeAction::operator traffic_simulator::lane_change::AbsoluteTarget() const
{
  return traffic_simulator::lane_change::AbsoluteTarget(
    boost::lexical_cast<std::int64_t>(lane_change_target.as<AbsoluteTargetLane>().value),
    target_lane_offset);
}

LaneChangeAction::operator traffic_simulator::lane_change::RelativeTarget() const
{
  return traffic_simulator::lane_change::RelativeTarget(
    lane_change_target.template as<RelativeTargetLane>().entity_ref,
    static_cast<traffic_simulator::lane_change::Direction>(
      lane_change_target.as<RelativeTargetLane>()),
    std::abs(lane_change_target.as<RelativeTargetLane>().value),  //
    target_lane_offset);
}
}  // namespace syntax
}  // namespace openscenario_interpreter

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

#ifndef OPENSCENARIO_INTERPRETER__SYNTAX__BOUNDING_BOX_HPP_
#define OPENSCENARIO_INTERPRETER__SYNTAX__BOUNDING_BOX_HPP_

#include <openscenario_interpreter/scope.hpp>
#include <openscenario_interpreter/syntax/center.hpp>
#include <openscenario_interpreter/syntax/dimensions.hpp>
#include <pugixml.hpp>
#include <traffic_simulator_msgs/msg/bounding_box.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
/* ---- BoundingBox ------------------------------------------------------------
 *
 *  <xsd:complexType name="BoundingBox">
 *    <xsd:all>
 *      <xsd:element name="Center" type="Center"/>
 *      <xsd:element name="Dimensions" type="Dimensions"/>
 *    </xsd:all>
 *  </xsd:complexType>
 *
 * -------------------------------------------------------------------------- */
struct BoundingBox
{
  const Center center;

  const Dimensions dimensions;

  BoundingBox() = default;

  explicit BoundingBox(const pugi::xml_node &, Scope &);

  explicit operator traffic_simulator_msgs::msg::BoundingBox() const;
};
}  // namespace syntax
}  // namespace openscenario_interpreter

#endif  // OPENSCENARIO_INTERPRETER__SYNTAX__BOUNDING_BOX_HPP_

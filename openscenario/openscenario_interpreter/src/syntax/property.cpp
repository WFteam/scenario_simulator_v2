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

#include <openscenario_interpreter/reader/attribute.hpp>
#include <openscenario_interpreter/syntax/boolean.hpp>
#include <openscenario_interpreter/syntax/property.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
Property::Property(const pugi::xml_node & node, Scope & scope)
: name(readAttribute<String>("name", node, scope)),  //
  value(readAttribute<String>("value", node, scope))
{
}

Property::operator bool() const { return value.empty() ? Boolean() : Boolean(value); }
}  // namespace syntax
}  // namespace openscenario_interpreter

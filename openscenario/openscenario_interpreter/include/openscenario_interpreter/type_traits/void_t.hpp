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

#ifndef OPENSCENARIO_INTERPRETER__TYPE_TRAITS__VOID_T_HPP_
#define OPENSCENARIO_INTERPRETER__TYPE_TRAITS__VOID_T_HPP_

#include <type_traits>

namespace openscenario_interpreter
{
inline namespace type_traits
{
template <typename...>
using void_t = void;
}
}  // namespace openscenario_interpreter

#endif  // OPENSCENARIO_INTERPRETER__TYPE_TRAITS__VOID_T_HPP_

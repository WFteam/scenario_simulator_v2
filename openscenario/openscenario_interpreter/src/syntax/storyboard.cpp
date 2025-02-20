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
#include <openscenario_interpreter/reader/element.hpp>
#include <openscenario_interpreter/syntax/scenario_object.hpp>
#include <openscenario_interpreter/syntax/story.hpp>
#include <openscenario_interpreter/syntax/storyboard.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
Storyboard::Storyboard(const pugi::xml_node & node, Scope & scope)
: Scope("Storyboard", scope),  // FIXME DIRTY HACK
  StoryboardElement(readElement<Trigger>("StopTrigger", node, local())),
  init(readElement<Init>("Init", node, local()))
{
  traverse<1, unbounded>(node, "Story", [&](auto && node) {
    return elements.push_back(readStoryboardElement<Story>(node, local()));
  });

  if (not init.endsImmediately()) {
    throw SemanticError("Init.Actions should end immediately");
  }
}

auto Storyboard::run() -> void
{
  if (engaged) {
    for (auto && story : elements) {
      story.evaluate();
    }
  } else if (std::all_of(  // XXX DIRTY HACK!!!
               std::cbegin(global().entities), std::cend(global().entities),
               [&](const auto & each) {
                 return not std::get<1>(each).template as<ScenarioObject>().is_added or
                        openscenario_interpreter::ready(std::get<0>(each));
               })) {
    for (const auto & each : global().entities) {
      if (std::get<1>(each).template as<ScenarioObject>().is_added) {
        engage(std::get<0>(each));
      }
    }
    engaged = true;
  } else {
    throw common::AutowareError(
      "Autoware did not reach an engageable state within the specified time "
      "(initialize_duration).");
  }
}

auto Storyboard::start() -> void
{
  init.evaluate();  // NOTE RENAME TO 'start'?
}

auto operator<<(nlohmann::json & json, const Storyboard & datum) -> nlohmann::json &
{
  json["currentState"] = boost::lexical_cast<std::string>(datum.state());

  json["Init"] << datum.init;

  json["Story"] = nlohmann::json::array();

  for (const auto & story : datum.elements) {
    nlohmann::json each;
    each << story.as<Story>();
    json["Story"].push_back(each);
  }

  return json;
}
}  // namespace syntax
}  // namespace openscenario_interpreter

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

#define OPENSCENARIO_INTERPRETER_NO_EXTENSION

#include <algorithm>
#include <nlohmann/json.hpp>
#include <openscenario_interpreter/openscenario_interpreter.hpp>
#include <openscenario_interpreter/record.hpp>
#include <openscenario_interpreter/syntax/object_controller.hpp>
#include <openscenario_interpreter/syntax/scenario_definition.hpp>
#include <openscenario_interpreter/utility/overload.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#define DECLARE_PARAMETER(IDENTIFIER) \
  declare_parameter<decltype(IDENTIFIER)>(#IDENTIFIER, IDENTIFIER)

#define GET_PARAMETER(IDENTIFIER) get_parameter(#IDENTIFIER, IDENTIFIER)

namespace openscenario_interpreter
{
Interpreter::Interpreter(const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("openscenario_interpreter", options),
  publisher_of_context(create_publisher<Context>("context", rclcpp::QoS(1).transient_local())),
  intended_result("success"),
  local_frame_rate(30),
  local_real_time_factor(1.0),
  osc_path(""),
  output_directory("/tmp")
{
  DECLARE_PARAMETER(intended_result);
  DECLARE_PARAMETER(local_frame_rate);
  DECLARE_PARAMETER(local_real_time_factor);
  DECLARE_PARAMETER(osc_path);
  DECLARE_PARAMETER(output_directory);
}

auto Interpreter::currentLocalFrameRate() const -> std::chrono::milliseconds
{
  return std::chrono::milliseconds(static_cast<unsigned int>(1 / local_frame_rate * 1000));
}

auto Interpreter::currentScenarioDefinition() const -> const std::shared_ptr<ScenarioDefinition> &
{
  return scenarios.front();
}

auto Interpreter::isAnErrorIntended() const -> bool { return intended_result == "error"; }

auto Interpreter::isFailureIntended() const -> bool { return intended_result == "failure"; }

auto Interpreter::isSuccessIntended() const -> bool { return intended_result == "success"; }

auto Interpreter::makeCurrentConfiguration() const -> traffic_simulator::Configuration
{
  const auto logic_file = currentScenarioDefinition()->road_network.logic_file;

  auto configuration = traffic_simulator::Configuration(
    logic_file.isDirectory() ? logic_file : logic_file.filepath.parent_path());
  {
    configuration.auto_sink = false;

    configuration.initialize_duration =
      ObjectController::ego_count > 0 ? getParameter<int>("initialize_duration") : 0;

    configuration.scenario_path = osc_path;

    // XXX DIRTY HACK!!!
    if (not logic_file.isDirectory() and logic_file.filepath.extension() == ".osm") {
      configuration.lanelet2_map_file = logic_file.filepath.filename().string();
    }
  }

  return configuration;
}

auto Interpreter::on_configure(const rclcpp_lifecycle::State &) -> Result
{
  return withExceptionHandler(
    [](auto &&...) {
      return Interpreter::Result::FAILURE;  // => Unconfigured
    },
    [this]() {
      /* ---- NOTE -------------------------------------------------------------
       *
       *  The scenario_test_runner that launched this node considers that "the
       *  scenario is not expected to finish" or "an abnormality has occurred
       *  that prevents the interpreter from terminating itself" after the
       *  specified time (specified by --global-timeout), and deactivates this
       *  node.
       *
       * -------------------------------------------------------------------- */
      result = common::junit::Failure(
        "Timeout",
        "The simulation time has exceeded the time specified by the scenario_test_runner.");

      std::this_thread::sleep_for(std::chrono::seconds(1));  // NOTE: Wait for parameters to be set.

      GET_PARAMETER(intended_result);
      GET_PARAMETER(local_frame_rate);
      GET_PARAMETER(local_real_time_factor);
      GET_PARAMETER(osc_path);
      GET_PARAMETER(output_directory);

      script = std::make_shared<OpenScenario>(osc_path);

      if (script->category.is<ScenarioDefinition>()) {
        scenarios = {std::dynamic_pointer_cast<ScenarioDefinition>(script->category)};
      } else {
        throw SyntaxError("ParameterValueDistributionDefinition is not yet supported.");
      }

      return Interpreter::Result::SUCCESS;  // => Inactive
    });
}

auto Interpreter::on_activate(const rclcpp_lifecycle::State &) -> Result
{
  auto evaluateStoryboard = [this]() {
    withExceptionHandler(
      [this](auto &&...) { deactivate(); },
      [this]() {
        if (currentScenarioDefinition()) {
          const auto evaluate_time = execution_timer.invoke("evaluate", [&] {
            currentScenarioDefinition()->evaluate();
            publishCurrentContext();
            return 0 <= getCurrentTime();  // statistics only if 0 <= getCurrentTime()
          });

          if (0 <= getCurrentTime() and currentLocalFrameRate() < evaluate_time) {
            RCLCPP_WARN_STREAM(
              get_logger(),
              "Your machine is not powerful enough to run the scenario at the specified "
              "frame rate ("
                << currentLocalFrameRate().count()
                << " Hz). We recommend that you reduce the frame rate to "
                << 1000.0 / execution_timer.getStatistics("evaluate")
                              .max<std::chrono::milliseconds>()
                              .count()
                << " or less.");
          }
        } else {
          throw Error("No script evaluable");
        }
      });
  };

  if (scenarios.empty()) {
    return Result::FAILURE;
  } else {
    return withExceptionHandler(
      [this](auto &&...) { return Interpreter::Result::ERROR; },
      [&]() {
        if (getParameter<bool>("record", true)) {
          record::start(
            "-a", "-o", boost::filesystem::path(osc_path).replace_extension("").string());
        }

        connect(shared_from_this(), makeCurrentConfiguration());

        initialize(local_real_time_factor, 1 / local_frame_rate * local_real_time_factor);

        execution_timer.clear();

        publisher_of_context->on_activate();

        assert(publisher_of_context->is_activated());

        timer = create_wall_timer(currentLocalFrameRate(), evaluateStoryboard);

        return Interpreter::Result::SUCCESS;  // => Active
      });
  }
}

auto Interpreter::on_deactivate(const rclcpp_lifecycle::State &) -> Result
{
  timer.reset();  // Stop scenario evaluation

  publisher_of_context->on_deactivate();

  disconnect();  // Deactivate traffic_simulator

  scenarios.pop_front();

  // NOTE: Error on simulation is not error of the interpreter; so we print error messages into INFO_STREAM.
  boost::apply_visitor(
    overload(
      [&](const common::junit::Pass & result) { RCLCPP_INFO_STREAM(get_logger(), result); },
      [&](const common::junit::Failure & result) { RCLCPP_INFO_STREAM(get_logger(), result); },
      [&](const common::junit::Error & result) { RCLCPP_INFO_STREAM(get_logger(), result); }),
    result);

  if (getParameter<bool>("record", true)) {
    record::stop();
  }

  return Interpreter::Result::SUCCESS;  // => Inactive
}

auto Interpreter::on_cleanup(const rclcpp_lifecycle::State &) -> Result
{
  return Interpreter::Result::SUCCESS;  // => Unconfigured
}

auto Interpreter::on_error(const rclcpp_lifecycle::State &) -> Result
{
  deactivate();  // DIRTY HACK!!!

  return Interpreter::Result::SUCCESS;  // => Unconfigured
}

auto Interpreter::on_shutdown(const rclcpp_lifecycle::State &) -> Result
{
  timer.reset();

  return Interpreter::Result::SUCCESS;  // => Finalized
}

auto Interpreter::publishCurrentContext() const -> void
{
  Context context;
  {
    nlohmann::json json;
    context.stamp = now();
    context.data = (json << *script).dump();
    context.time = getCurrentTime();
  }

  publisher_of_context->publish(context);
}
}  // namespace openscenario_interpreter

RCLCPP_COMPONENTS_REGISTER_NODE(openscenario_interpreter::Interpreter)

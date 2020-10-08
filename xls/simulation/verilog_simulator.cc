// Copyright 2020 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/simulation/verilog_simulator.h"

#include "absl/flags/flag.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/file/temp_directory.h"
#include "xls/common/file/temp_file.h"
#include "xls/common/logging/log_lines.h"
#include "xls/common/status/status_macros.h"
#include "xls/common/subprocess.h"
#include "re2/re2.h"

namespace xls {
namespace verilog {
namespace {

absl::StatusOr<std::vector<Observation>> StdoutToObservations(
    absl::string_view output, const NameToBitCount& to_observe) {
  std::vector<Observation> result;
  auto error = [](absl::string_view line, absl::string_view message) {
    return absl::InternalError(
        absl::StrCat("Simulation produced invalid monitoring line: \"", line,
                     "\" :: ", message));
  };
  for (absl::string_view line : absl::StrSplit(output, '\n')) {
    line = absl::StripAsciiWhitespace(line);
    if (line.empty()) {
      continue;
    }

    if (!RE2::FullMatch(line, R"(^\s*[0-9]+\s*:.*)")) {
      // Skip lines which do not begin with a numeric time value followed by a
      // colon.
      continue;
    }

    std::vector<absl::string_view> pieces = absl::StrSplit(line, ':');
    if (pieces.size() != 2) {
      return error(line, "missing time-delimiting ':'");
    }

    int64 time;
    if (!absl::SimpleAtoi(pieces[0], &time)) {
      return error(line, "invalid simulation time value");
    }

    // Turn all of the print-outs at this time into "observations".
    std::vector<absl::string_view> observed = absl::StrSplit(pieces[1], ';');
    for (absl::string_view observation : observed) {
      std::string name;
      uint64 value;
      if (!RE2::FullMatch(observation, "\\s*(\\w+) = ([0-9A-Fa-f]+)\\s*", &name,
                          RE2::Hex(&value))) {
        return error(line, "monitoring line did not match expected pattern");
      }
      auto it = to_observe.find(name);
      if (it == to_observe.end()) {
        continue;
      }
      int64 bit_count = it->second;
      result.push_back(Observation{time, name, UBits(value, bit_count)});
    }
  }
  return result;
}

}  // namespace

absl::StatusOr<std::pair<std::string, std::string>> VerilogSimulator::Run(
    absl::string_view text) const {
  return Run(text, /*includes=*/{});
}

absl::Status VerilogSimulator::RunSyntaxChecking(absl::string_view text) const {
  return RunSyntaxChecking(text, /*includes=*/{});
}

absl::StatusOr<std::vector<Observation>>
VerilogSimulator::SimulateCombinational(
    absl::string_view text, const NameToBitCount& to_observe) const {
  std::pair<std::string, std::string> stdout_stderr;
  XLS_ASSIGN_OR_RETURN(stdout_stderr, Run(text));
  return StdoutToObservations(stdout_stderr.first, to_observe);
}

VerilogSimulatorManager& GetVerilogSimulatorManagerSingleton() {
  static VerilogSimulatorManager* manager = new VerilogSimulatorManager;
  return *manager;
}

absl::StatusOr<VerilogSimulator*> VerilogSimulatorManager::GetVerilogSimulator(
    absl::string_view name) const {
  if (!simulators_.contains(name)) {
    if (simulator_names_.empty()) {
      return absl::NotFoundError(
          absl::StrFormat("No simulator found named \"%s\". No "
                          "simulators are registered. Was InitXls called?",
                          name));
    } else {
      return absl::NotFoundError(absl::StrFormat(
          "No simulator found named \"%s\". Available simulators: %s", name,
          absl::StrJoin(simulator_names_, ", ")));
    }
  }
  return simulators_.at(name).get();
}

absl::Status VerilogSimulatorManager::RegisterVerilogSimulator(
    absl::string_view name, std::unique_ptr<VerilogSimulator> simulator) {
  if (simulators_.contains(name)) {
    return absl::InternalError(
        absl::StrFormat("Simulator named %s already exists", name));
  }
  simulators_[name] = std::move(simulator);
  simulator_names_.push_back(std::string(name));
  std::sort(simulator_names_.begin(), simulator_names_.end());

  return absl::OkStatus();
}

}  // namespace verilog
}  // namespace xls

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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"
#include "xls/simulation/verilog_simulators.h"
#include "xls/simulation/verilog_test_base.h"

namespace xls {
namespace verilog {
namespace {

class VerilogSimulatorTest : public VerilogTestBase {};

// A trivial sanity test for the verilog simulators.
TEST_P(VerilogSimulatorTest, SimulatesSampleCombinationalTestbenchText) {
  std::string text = R"(module device_under_test(
  input [1:0] x,
  input [1:0] y,
  output [1:0] z,
  output c
);
  wire [2:0] temp;
  assign temp = x + y;
  assign c = temp[2:2];
  assign z = temp[1:0];
endmodule

module tb;
  reg [1:0] x = 2'd3;
  reg [1:0] y = 2'd3;
  wire [1:0] z;
  wire c;
  device_under_test dut(.x(x), .y(y), .z(z), .c(c));
  initial begin
    $monitor("%t: z = %h; c = %h", $time, z, c);
  end
endmodule
)";
  const NameToBitCount to_observe = {{"z", 2}, {"c", 1}};

  // Run the simulation and collect observations.
  XLS_ASSERT_OK_AND_ASSIGN(
      std::vector<Observation> observations,
      GetSimulator()->SimulateCombinational(text, to_observe));
  ASSERT_EQ(2, observations.size());

  // First observation (of z).
  EXPECT_EQ(0, observations[0].time);
  EXPECT_EQ("z", observations[0].name);
  EXPECT_EQ(UBits(2, /*bit_count=*/2), observations[0].value);

  // Second observation (of c, at same time).
  EXPECT_EQ("c", observations[1].name);
  EXPECT_EQ(UBits(1, /*bit_count=*/1), observations[1].value);
}

INSTANTIATE_TEST_SUITE_P(VerilogSimulatorTestInstantiation,
                         VerilogSimulatorTest,
                         testing::ValuesIn(kVerilogOnlySimulationTargets),
                         ParameterizedTestName<VerilogSimulatorTest>);

}  // namespace
}  // namespace verilog
}  // namespace xls

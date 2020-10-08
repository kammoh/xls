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

#include "xls/interpreter/proc_interpreter.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "xls/common/status/matchers.h"
#include "xls/ir/channel.h"
#include "xls/ir/function_builder.h"
#include "xls/ir/ir_test_base.h"

namespace xls {
namespace {

using status_testing::IsOkAndHolds;
using status_testing::StatusIs;
using ::testing::ElementsAre;
using ::testing::HasSubstr;

class ProcInterpreterTest : public IrTestBase {};

TEST_F(ProcInterpreterTest, ProcIota) {
  auto package = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(
      Channel * channel,
      package->CreateChannel("iota_out", ChannelKind::kSendOnly,
                             {DataElement{"data", package->GetBitsType(32)}},
                             ChannelMetadataProto()));

  // Create an output-only proc which counts up by 7 starting at 42.
  ProcBuilder pb("iota", /*init_value=*/Value(UBits(42, 32)),
                 /*token_name=*/"tok", /*state_name=*/"prev", package.get());
  BValue send_token =
      pb.Send(channel, pb.GetTokenParam(), {pb.GetStateParam()});
  BValue new_value = pb.Add(pb.GetStateParam(), pb.Literal(UBits(7, 32)));
  XLS_ASSERT_OK(pb.Build(send_token, new_value).status());

  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ChannelQueueManager> queue_manager,
      ChannelQueueManager::Create(/*rx_only_queues=*/{}, package.get()));
  ProcInterpreter interpreter(FindProc("iota", package.get()),
                              queue_manager.get());
  ChannelQueue& ch0_queue = queue_manager->GetQueue(channel);

  ASSERT_TRUE(ch0_queue.empty());

  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  EXPECT_TRUE(interpreter.IsIterationComplete());
  EXPECT_EQ(ch0_queue.size(), 1);
  EXPECT_FALSE(ch0_queue.empty());
  EXPECT_THAT(ch0_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(42, 32)))));
  EXPECT_EQ(ch0_queue.size(), 0);
  EXPECT_TRUE(ch0_queue.empty());

  // Run three times. Should enqueue three values in the output queue.
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));

  EXPECT_EQ(ch0_queue.size(), 3);

  EXPECT_THAT(ch0_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(49, 32)))));
  EXPECT_THAT(ch0_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(56, 32)))));
  EXPECT_THAT(ch0_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(63, 32)))));

  EXPECT_TRUE(ch0_queue.empty());
}

TEST_F(ProcInterpreterTest, ProcWhichReturnsPreviousResults) {
  Package package(TestName());
  ProcBuilder pb("prev", /*init_value=*/Value(UBits(55, 32)),
                 /*token_name=*/"tok", /*state_name=*/"prev", &package);
  XLS_ASSERT_OK_AND_ASSIGN(
      Channel * ch_in,
      package.CreateChannel("in", ChannelKind::kSendReceive,
                            {DataElement{"data", package.GetBitsType(32)}},
                            ChannelMetadataProto()));
  XLS_ASSERT_OK_AND_ASSIGN(
      Channel * ch_out,
      package.CreateChannel("out", ChannelKind::kSendOnly,
                            {DataElement{"data", package.GetBitsType(32)}},
                            ChannelMetadataProto()));

  // Build a proc which receives a value and saves it, and sends the value
  // received in the previous iteration.
  BValue token_input = pb.Receive(ch_in, pb.GetTokenParam());
  BValue recv_token = pb.TupleIndex(token_input, 0);
  BValue input = pb.TupleIndex(token_input, 1);
  BValue send_token = pb.Send(ch_out, recv_token, {pb.GetStateParam()});
  XLS_ASSERT_OK_AND_ASSIGN(Proc * proc, pb.Build(send_token, input));

  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ChannelQueueManager> queue_manager,
      ChannelQueueManager::Create(/*rx_only_queues=*/{}, &package));

  ProcInterpreter interpreter(proc, queue_manager.get());
  ChannelQueue& input_queue = queue_manager->GetQueue(ch_in);
  ChannelQueue& output_queue = queue_manager->GetQueue(ch_out);

  ASSERT_TRUE(input_queue.empty());
  ASSERT_TRUE(output_queue.empty());

  // First invocation of RunIterationUntilCompleteOrBlocked should block on
  // waiting for input on the "in" channel.
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = false,
                                              .progress_made = true,
                                              .blocked_channels = {ch_in}}));
  EXPECT_FALSE(interpreter.IsIterationComplete());

  // Blocked on the receive so no progress should be made if you try to resume
  // execution again.
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = false,
                                              .progress_made = false,
                                              .blocked_channels = {ch_in}}));
  EXPECT_FALSE(interpreter.IsIterationComplete());

  // Enqueue something into the input queue.
  XLS_ASSERT_OK(input_queue.Enqueue({Value(UBits(42, 32))}));
  EXPECT_EQ(input_queue.size(), 1);
  EXPECT_TRUE(output_queue.empty());

  // It can now continue until complete.
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  EXPECT_TRUE(interpreter.IsIterationComplete());

  EXPECT_TRUE(input_queue.empty());
  EXPECT_EQ(output_queue.size(), 1);

  EXPECT_THAT(output_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(55, 32)))));
  EXPECT_TRUE(output_queue.empty());

  // Now run the next iteration. It should spit out the value we fed in during
  // the last iteration (42).
  XLS_ASSERT_OK(input_queue.Enqueue({Value(UBits(123, 32))}));
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  EXPECT_THAT(output_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(42, 32)))));
}

TEST_F(ProcInterpreterTest, ReceiveIfProc) {
  // Create a proc which has a receive_if which fires every other
  // iteration. Receive_if value is unconditionally sent over a different
  // channel.
  Package package(TestName());
  ProcBuilder pb("send_if", /*init_value=*/Value(UBits(1, 1)),
                 /*token_name=*/"tok", /*state_name=*/"st", &package);
  XLS_ASSERT_OK_AND_ASSIGN(
      Channel * ch_in,
      package.CreateChannel("in", ChannelKind::kSendReceive,
                            {DataElement{"data", package.GetBitsType(32)}},
                            ChannelMetadataProto()));
  XLS_ASSERT_OK_AND_ASSIGN(
      Channel * ch_out,
      package.CreateChannel("out", ChannelKind::kSendOnly,
                            {DataElement{"data", package.GetBitsType(32)}},
                            ChannelMetadataProto()));

  BValue receive_if = pb.ReceiveIf(ch_in, /*token=*/pb.GetTokenParam(),
                                   /*pred=*/pb.GetStateParam());
  BValue rx_token = pb.TupleIndex(receive_if, 0);
  BValue rx_data = pb.TupleIndex(receive_if, 1);
  BValue send = pb.Send(ch_out, rx_token, {rx_data});
  // Next state value is the inverse of the current state value.
  XLS_ASSERT_OK_AND_ASSIGN(Proc * proc,
                           pb.Build(send, pb.Not(pb.GetStateParam())));

  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ChannelQueueManager> queue_manager,
      ChannelQueueManager::Create(/*rx_only_queues=*/{}, &package));

  ProcInterpreter interpreter(proc, queue_manager.get());
  ChannelQueue& input_queue = queue_manager->GetQueue(ch_in);
  ChannelQueue& output_queue = queue_manager->GetQueue(ch_out);

  ASSERT_TRUE(input_queue.empty());
  ASSERT_TRUE(output_queue.empty());

  // Enqueue a single value into the input queue.
  XLS_ASSERT_OK(input_queue.Enqueue({Value(UBits(42, 32))}));

  // In the first iteration, the receive_if should dequeue a value because the
  // proc state value (which is the receive_if predicate) is initialized to
  // true.
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  EXPECT_THAT(output_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(42, 32)))));

  // The second iteration should not dequeue anything as the receive_if
  // predicate is now false. The data value of the receive_if (which is sent
  // over the output channel) should be zeros.
  ASSERT_TRUE(input_queue.empty());
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  EXPECT_THAT(output_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(0, 32)))));

  // The third iteration should again dequeue a value.
  XLS_ASSERT_OK(input_queue.Enqueue({Value(UBits(123, 32))}));
  ASSERT_THAT(
      interpreter.RunIterationUntilCompleteOrBlocked(),
      IsOkAndHolds(ProcInterpreter::RunResult{.iteration_complete = true,
                                              .progress_made = true,
                                              .blocked_channels = {}}));
  EXPECT_THAT(output_queue.Dequeue(),
              IsOkAndHolds(ElementsAre(Value(UBits(123, 32)))));
}

TEST_F(ProcInterpreterTest, SendIfProc) {
  // Create an output-only proc with a by-one-counter which sends only
  // even values over a send_if.
  Package package(TestName());
  XLS_ASSERT_OK_AND_ASSIGN(
      Channel * channel,
      package.CreateChannel("even_out", ChannelKind::kSendOnly,
                            {DataElement{"data", package.GetBitsType(32)}},
                            ChannelMetadataProto()));

  ProcBuilder pb("even", /*init_value=*/Value(UBits(0, 32)),
                 /*token_name=*/"tok", /*state_name=*/"prev", &package);
  BValue is_even =
      pb.Eq(pb.BitSlice(pb.GetStateParam(), /*start=*/0, /*width=*/1),
            pb.Literal(UBits(0, 1)));
  BValue send_if =
      pb.SendIf(channel, pb.GetTokenParam(), is_even, {pb.GetStateParam()});
  BValue new_value = pb.Add(pb.GetStateParam(), pb.Literal(UBits(1, 32)));
  XLS_ASSERT_OK(pb.Build(send_if, new_value).status());

  XLS_ASSERT_OK_AND_ASSIGN(
      std::unique_ptr<ChannelQueueManager> queue_manager,
      ChannelQueueManager::Create(/*rx_only_queues=*/{}, &package));
  ProcInterpreter interpreter(FindProc("even", &package), queue_manager.get());

  ChannelQueue& queue = queue_manager->GetQueue(channel);

  XLS_ASSERT_OK(interpreter.RunIterationUntilCompleteOrBlocked().status());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_THAT(queue.Dequeue(), IsOkAndHolds(ElementsAre(Value(UBits(0, 32)))));

  XLS_ASSERT_OK(interpreter.RunIterationUntilCompleteOrBlocked().status());
  EXPECT_TRUE(queue.empty());

  XLS_ASSERT_OK(interpreter.RunIterationUntilCompleteOrBlocked().status());
  EXPECT_THAT(queue.Dequeue(), IsOkAndHolds(ElementsAre(Value(UBits(2, 32)))));

  XLS_ASSERT_OK(interpreter.RunIterationUntilCompleteOrBlocked().status());
  EXPECT_TRUE(queue.empty());

  XLS_ASSERT_OK(interpreter.RunIterationUntilCompleteOrBlocked().status());
  EXPECT_THAT(queue.Dequeue(), IsOkAndHolds(ElementsAre(Value(UBits(4, 32)))));
}

}  // namespace
}  // namespace xls

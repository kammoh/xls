// Copyright 2021 The XLS Authors
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

#include "xls/contrib/xlscc/unit_tests/unit_test.h"

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "clang/include/clang/AST/Decl.h"
#include "xls/common/file/get_runfile_path.h"
#include "xls/common/logging/log_sink_registry.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/xlscc/translator.h"
#include "xls/contrib/xlscc/xlscc_logging.h"
#include "xls/interpreter/function_interpreter.h"
#include "xls/interpreter/interpreter_proc_runtime.h"
#include "xls/interpreter/serial_proc_runtime.h"
#include "xls/ir/nodes.h"
#include "xls/ir/package.h"
#include "xls/ir/proc.h"
#include "xls/ir/source_location.h"
#include "xls/ir/value.h"
#include "xls/ir/value_helpers.h"

using testing::Optional;

const int determinism_test_repeat_count = 3;

CapturedLogEntry::CapturedLogEntry() = default;

CapturedLogEntry::CapturedLogEntry(const ::xls::LogEntry& entry)
    : text_message(entry.text_message()),
      log_severity(entry.log_severity()),
      verbosity(entry.verbosity()),
      source_filename(entry.source_filename()),
      source_basename(entry.source_basename()),
      source_line(entry.source_line()),
      prefix(entry.prefix()) {}

XlsccTestBase::XlsccTestBase() { ::xls::AddLogSink(this); }

XlsccTestBase::~XlsccTestBase() { ::xls::RemoveLogSink(this); }

void XlsccTestBase::Send(const ::xls::LogEntry& entry) {
  log_entries_.emplace_back(entry);
}

void XlsccTestBase::Run(const absl::flat_hash_map<std::string, uint64_t>& args,
                        uint64_t expected, std::string_view cpp_source,
                        xabsl::SourceLocation loc,
                        std::vector<std::string_view> clang_argv,
                        int64_t max_unroll_iters) {
  if (XLS_VLOG_IS_ON(1)) {
    std::ostringstream input_str;
    for (const auto& [key, val] : args) {
      input_str << key << ":" << val << " ";
    }
    XLS_VLOG(1) << absl::StrFormat("Run test in (%s) out %i", input_str.str(),
                                   expected)
                << std::endl;
  }
  testing::ScopedTrace trace(loc.file_name(), loc.line(), "Run failed");
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string ir, SourceToIr(cpp_source, nullptr, clang_argv,
                                 /*io_test_mode=*/false, max_unroll_iters));
  RunAndExpectEq(args, expected, ir, false, false, loc);
}

void XlsccTestBase::Run(
    const absl::flat_hash_map<std::string, xls::Value>& args,
    xls::Value expected, std::string_view cpp_source, xabsl::SourceLocation loc,
    std::vector<std::string_view> clang_argv, int64_t max_unroll_iters) {
  testing::ScopedTrace trace(loc.file_name(), loc.line(), "Run failed");
  XLS_ASSERT_OK_AND_ASSIGN(
      std::string ir, SourceToIr(cpp_source, nullptr, clang_argv,
                                 /*io_test_mode=*/false, max_unroll_iters));
  RunAndExpectEq(args, expected, ir, false, false, loc);
}

void XlsccTestBase::RunAcDatatypeTest(
    const absl::flat_hash_map<std::string, uint64_t>& args, uint64_t expected,
    std::string_view cpp_source, xabsl::SourceLocation loc) {
  XLS_ASSERT_OK_AND_ASSIGN(std::vector<std::string> clang_args,
                           GetClangArgForIntTest());
  std::vector<std::string_view> clang_argv(clang_args.begin(),
                                           clang_args.end());
  Run(args, expected, cpp_source, loc, clang_argv);
}

absl::StatusOr<std::vector<std::string>> XlsccTestBase::GetClangArgForIntTest()
    const {
  XLS_ASSIGN_OR_RETURN(std::string ac_int_path,
                       xls::GetXlsRunfilePath("external/com_github_hlslibs_ac_types/include/ac_int.h"));
  XLS_ASSIGN_OR_RETURN(
      std::string xls_int_path,
      xls::GetXlsRunfilePath("xls/contrib/xlscc/synth_only/xls_int.h"));

  // Get the path that includes the ac_datatypes folder, so that the
  //  ac_datatypes headers can be included with the form:
  // #include "external/com_github_hlslibs_ac_types/include/foo.h"
  auto ac_int_dir = std::filesystem::path(ac_int_path);
  ac_int_dir = ac_int_dir.parent_path().parent_path();

  auto xls_int_dir = std::filesystem::path(xls_int_path).parent_path();

  std::vector<std::string> argv;
  argv.push_back(std::string("-I") + xls_int_dir.string());
  argv.push_back(std::string("-I") + ac_int_dir.string());
  argv.push_back("-D__SYNTHESIS__");
  return argv;
}

void XlsccTestBase::RunWithStatics(
    const absl::flat_hash_map<std::string, xls::Value>& args,
    const absl::Span<xls::Value>& expected_outputs, std::string_view cpp_source,
    xabsl::SourceLocation loc) {
  testing::ScopedTrace trace(loc.file_name(), loc.line(),
                             "RunWithStatics failed");

  xlscc::GeneratedFunction* pfunc = nullptr;

  XLS_ASSERT_OK_AND_ASSIGN(std::string ir, SourceToIr(cpp_source, &pfunc));

  ASSERT_NE(pfunc, nullptr);

  ASSERT_GT(pfunc->static_values.size(), 0);

  ASSERT_EQ(pfunc->io_ops.size(), 0);

  XLS_ASSERT_OK_AND_ASSIGN(package_, ParsePackage(ir));

  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * top_func,
                           package_->GetTopAsFunction());

  const absl::Span<xls::Param* const> params = top_func->params();

  ASSERT_GE(params.size(), pfunc->static_values.size());

  absl::flat_hash_map<const clang::NamedDecl*, std::string> static_param_names;
  absl::flat_hash_map<const clang::NamedDecl*, xls::Value> static_state;
  for (const xlscc::SideEffectingParameter& param :
       pfunc->side_effecting_parameters) {
    XLS_CHECK(param.type == xlscc::SideEffectingParameterType::kStatic);
    const xls::Value& init_value =
        pfunc->static_values.at(param.static_value).rvalue();
    static_param_names[param.static_value] = param.param_name;
    XLS_CHECK(!static_state.contains(param.static_value));
    static_state[param.static_value] = init_value;
  }

  for (const xls::Value& expected_output : expected_outputs) {
    absl::flat_hash_map<std::string, xls::Value> args_with_statics = args;

    for (const clang::NamedDecl* decl :
         pfunc->GetDeterministicallyOrderedStaticValues()) {
      args_with_statics[static_param_names.at(decl)] = static_state.at(decl);
    }

    XLS_ASSERT_OK_AND_ASSIGN(xls::Value actual,
                             DropInterpreterEvents(xls::InterpretFunctionKwargs(
                                 top_func, args_with_statics)));
    XLS_ASSERT_OK_AND_ASSIGN(std::vector<xls::Value> returns,
                             actual.GetElements());
    ASSERT_EQ(returns.size(), pfunc->static_values.size() + 1);

    {
      int i = 0;
      for (const clang::NamedDecl* decl :
           pfunc->GetDeterministicallyOrderedStaticValues()) {
        XLS_CHECK(static_state.contains(decl));
        static_state[decl] = returns[i++];
      }
    }

    EXPECT_EQ(returns[pfunc->static_values.size()], expected_output);
  }
}

template <typename... Args>
std::string ErrorMessage(const xls::SourceInfo& loc,
                         const absl::FormatSpec<Args...>& format,
                         const Args&... args) {
  std::string result = absl::StrFormat(format, args...);
  for (const xls::SourceLocation& location : loc.locations) {
    absl::StrAppend(&result, "\n", location.ToString());
  }
  return result;
}

absl::Status XlsccTestBase::ScanFile(
    xls::TempFile& temp, std::vector<std::string_view> clang_argv,
    bool io_test_mode, bool error_on_init_interval, xls::SourceLocation loc,
    bool fail_xlscc_check, int64_t max_unroll_iters,
    const char* top_class_name) {
  auto parser = std::make_unique<xlscc::CCParser>();
  XLS_RETURN_IF_ERROR(ScanTempFileWithContent(
      temp, clang_argv, parser.get(), /*top_name=*/"my_package",
      /*top_class_name=*/top_class_name));
  // When loop unrolling is failing, it tends to run slowly.
  // Since there are several unit tests to check the failing case, the maximum
  // loop iterations is set lower than in the main tool interface to make
  // the test run in a reasonable time.
  translator_.reset(new xlscc::Translator(
      error_on_init_interval,
      /*max_unroll_iters=*/(max_unroll_iters > 0) ? max_unroll_iters : 100,
      /*warn_unroll_iters=*/100, /*z3_rlimit=*/-1,
      /*op_ordering=*/xlscc::IOOpOrdering::kLexical, std::move(parser)));
  if (io_test_mode) {
    translator_->SetIOTestMode();
  }
  if (fail_xlscc_check) {
    auto source_info = xls::SourceInfo(loc);
    XLSCC_CHECK(false, source_info);
  }
  return absl::OkStatus();
}

absl::Status XlsccTestBase::ScanFile(
    std::string_view cpp_src, std::vector<std::string_view> clang_argv,
    bool io_test_mode, bool error_on_init_interval, xls::SourceLocation loc,
    bool fail_xlscc_check, int64_t max_unroll_iters,
    const char* top_class_name) {
  XLS_ASSIGN_OR_RETURN(xls::TempFile temp,
                       xls::TempFile::CreateWithContent(cpp_src, ".cc"));
  return ScanFile(temp, clang_argv, io_test_mode, error_on_init_interval, loc,
                  fail_xlscc_check, max_unroll_iters, top_class_name);
}

/* static */ absl::Status XlsccTestBase::ScanTempFileWithContent(
    std::string_view cpp_src, std::vector<std::string_view> argv,
    xlscc::CCParser* translator, const char* top_name,
    const char* top_class_name) {
  XLS_ASSIGN_OR_RETURN(xls::TempFile temp,
                       xls::TempFile::CreateWithContent(cpp_src, ".cc"));
  return ScanTempFileWithContent(temp, argv, translator, top_name,
                                 top_class_name);
}

/* static */ absl::Status XlsccTestBase::ScanTempFileWithContent(
    xls::TempFile& temp, std::vector<std::string_view> argv,
    xlscc::CCParser* translator, const char* top_name,
    const char* top_class_name) {
  std::string ps = temp.path();

  absl::Status ret;
  argv.push_back("-Werror");
  argv.push_back("-Wall");
  argv.push_back("-Wno-unknown-pragmas");
  if (top_name != nullptr) {
    XLS_RETURN_IF_ERROR(translator->SelectTop(top_name, top_class_name));
  }
  XLS_RETURN_IF_ERROR(translator->ScanFile(
      temp.path().c_str(), argv.empty()
                               ? absl::Span<std::string_view>()
                               : absl::MakeSpan(&argv[0], argv.size())));
  return absl::OkStatus();
}

absl::StatusOr<std::string> XlsccTestBase::SourceToIr(
    std::string_view cpp_src, xlscc::GeneratedFunction** pfunc,
    const std::vector<std::string_view>& clang_argv, bool io_test_mode,
    int64_t max_unroll_iters) {
  XLS_ASSIGN_OR_RETURN(xls::TempFile temp,
                       xls::TempFile::CreateWithContent(cpp_src, ".cc"));
  XLS_ASSIGN_OR_RETURN(
      std::string ir,
      SourceToIr(temp, pfunc, clang_argv, io_test_mode, max_unroll_iters));
  return ir;
}

absl::StatusOr<std::string> XlsccTestBase::SourceToIr(
    xls::TempFile& temp, xlscc::GeneratedFunction** pfunc,
    const std::vector<std::string_view>& clang_argv, bool io_test_mode,
    int64_t max_unroll_iters) {
  std::list<std::string> ir_texts;
  std::string ret_text;

  for (size_t test_i = 0; test_i < determinism_test_repeat_count; ++test_i) {
    log_entries_.clear();
    XLS_RETURN_IF_ERROR(ScanFile(temp, clang_argv, io_test_mode,
                                 /*error_on_init_interval=*/false,
                                 /*loc=*/xls::SourceLocation(),
                                 /*fail_xlscc_check=*/false, max_unroll_iters));
    package_.reset(new xls::Package("my_package"));
    absl::flat_hash_map<const clang::NamedDecl*, xlscc::ChannelBundle>
        top_channel_injections = {};
    XLS_ASSIGN_OR_RETURN(xlscc::GeneratedFunction * func,
                         translator_->GenerateIR_Top_Function(
                             package_.get(), top_channel_injections));
    XLS_RETURN_IF_ERROR(package_->SetTopByName(func->xls_func->name()));
    if (pfunc != nullptr) {
      *pfunc = func;
    }
    translator_->AddSourceInfoToPackage(*package_);
    ret_text = package_->DumpIr();
    ir_texts.push_back(ret_text);
  }

  // Determinism test
  for (const std::string& text : ir_texts) {
    EXPECT_EQ(text, ret_text) << "Failed determinism test";
  }
  return ret_text;
}

void XlsccTestBase::ProcTest(
    std::string content, std::optional<xlscc::HLSBlock> block_spec,
    const absl::flat_hash_map<std::string, std::list<xls::Value>>&
        inputs_by_channel,
    const absl::flat_hash_map<std::string, std::list<xls::Value>>&
        outputs_by_channel,
    const int min_ticks, const int max_ticks, int top_level_init_interval,
    const char* top_class_name, absl::Status expected_tick_status,
    const absl::flat_hash_map<std::string, xls::InterpreterEvents>&
        expected_events_by_proc_name) {
  std::list<std::string> ir_texts;
  std::string package_text;

  // file names are included in the package IR. Reuse same file name to
  // prevent non-determinism in IR text
  XLS_ASSERT_OK_AND_ASSIGN(xls::TempFile temp,
                           xls::TempFile::CreateWithContent(content, ".cc"));
  for (size_t test_i = 0; test_i < determinism_test_repeat_count; ++test_i) {
    log_entries_.clear();
    XLS_ASSERT_OK(ScanFile(temp,
                           /*clang_argv=*/{},
                           /*io_test_mode=*/false,
                           /*error_on_init_interval=*/false,
                           xls::SourceLocation(),
                           /*fail_xlscc_check=*/false,
                           /*max_unroll_iters=*/0,

                           /*top_class_name=*/top_class_name));

    package_.reset(new xls::Package("my_package"));
    if (block_spec.has_value()) {
      block_spec_ = block_spec.value();
      XLS_ASSERT_OK(translator_
                        ->GenerateIR_Block(package_.get(), block_spec.value(),
                                           top_level_init_interval)
                        .status());
    } else {
      XLS_ASSERT_OK(translator_
                        ->GenerateIR_BlockFromClass(package_.get(),
                                                    &block_spec_,
                                                    top_level_init_interval)
                        .status());
    }
    package_text = package_->DumpIr();
    ir_texts.push_back(package_text);
  }

  // Determinism test
  for (const std::string& text : ir_texts) {
    EXPECT_EQ(package_text, text) << "Failed determinism test";
  }

  XLS_LOG(INFO) << "Package IR: ";
  XLS_LOG(INFO) << package_text;

  std::vector<std::unique_ptr<xls::ChannelQueue>> queues;

  XLS_ASSERT_OK_AND_ASSIGN(
      auto interpreter,
      xls::CreateInterpreterSerialProcRuntime(package_.get()));

  auto print_list =
      [](const absl::flat_hash_map<std::string, std::list<xls::Value>>& l) {
        for (const auto& [name, values] : l) {
          std::ostringstream value_str;
          for (const xls::Value& value : values) {
            value_str << value.ToString() << " ";
          }
          XLS_LOG(INFO) << "-- " << name.c_str() << ": " << value_str.str();
        }
      };

  XLS_LOG(INFO) << "Inputs:";
  print_list(inputs_by_channel);
  XLS_LOG(INFO) << "Outputs:";
  print_list(outputs_by_channel);

  xls::ChannelQueueManager& queue_manager = interpreter->queue_manager();

  // Write all inputs.
  for (auto [ch_name, values] : inputs_by_channel) {
    XLS_ASSERT_OK_AND_ASSIGN(xls::ChannelQueue * queue,
                             queue_manager.GetQueueByName(ch_name));

    for (const xls::Value& value : values) {
      XLS_LOG(INFO) << absl::StrFormat("Writing %s on channel %s",
                                       value.ToString(),
                                       queue->channel()->name());
      XLS_ASSERT_OK(queue->Write(value));
    }
  }

  absl::flat_hash_map<std::string, std::list<xls::Value>>
      mutable_outputs_by_channel = outputs_by_channel;

  int tick = 1;
  for (; tick < max_ticks; ++tick) {
    XLS_LOG(INFO) << "Before tick " << tick;

    ASSERT_EQ(interpreter->Tick(), expected_tick_status);

    XLS_LOG(INFO) << "State after tick " << tick;
    for (const auto& proc : package_->procs()) {
      XLS_LOG(INFO) << absl::StrFormat(
          "[%s]: %s", proc->name(),
          absl::StrFormat(
              "{%s}", absl::StrJoin(interpreter->ResolveState(proc.get()), ", ",
                                    xls::ValueFormatter)));
    }

    // Check as we go
    bool all_output_channels_empty = true;
    for (auto& [ch_name, values] : mutable_outputs_by_channel) {
      XLS_ASSERT_OK_AND_ASSIGN(xls::Channel * ch_out,
                               package_->GetChannel(ch_name));
      xls::ChannelQueue& ch_out_queue = queue_manager.GetQueue(ch_out);

      while (!ch_out_queue.IsEmpty()) {
        const xls::Value& next_output = values.front();
        XLS_LOG(INFO) << "Checking output on channel: "
                      << ch_out_queue.channel()->name();
        EXPECT_THAT(ch_out_queue.Read(), Optional(next_output));
        values.pop_front();
      }

      all_output_channels_empty = all_output_channels_empty && values.empty();
    }
    if (all_output_channels_empty) {
      break;
    }
  }

  for (auto [ch_name, values] : outputs_by_channel) {
    XLS_ASSERT_OK_AND_ASSIGN(xls::Channel * ch_out,
                             package_->GetChannel(ch_name));
    xls::ChannelQueue& ch_out_queue = queue_manager.GetQueue(ch_out);

    EXPECT_EQ(ch_out_queue.GetSize(), 0);
  }

  EXPECT_GE(tick, min_ticks);
  EXPECT_LE(tick, max_ticks);

  for (const auto& [proc_name, events] : expected_events_by_proc_name) {
    XLS_ASSERT_OK_AND_ASSIGN(xls::Proc * proc, package_->GetProc(proc_name));
    EXPECT_EQ(events, interpreter->GetInterpreterEvents(proc));
  }
}

absl::StatusOr<uint64_t> XlsccTestBase::GetStateBitsForProcNameContains(
    std::string_view name_cont) {
  XLS_CHECK_NE(nullptr, package_.get());
  uint64_t ret = 0;
  xls::Proc* already_found = nullptr;
  for (std::unique_ptr<xls::Proc>& proc : package_->procs()) {
    if (absl::StrContains(proc->name(), name_cont)) {
      if (already_found != nullptr) {
        return absl::NotFoundError(absl::StrFormat(
            "Proc with name containing %s already found, %s vs %s", name_cont,
            already_found->name(), proc->name()));
      }
      for (xls::Param* state_param : proc->StateParams()) {
        ret += state_param->GetType()->GetFlatBitCount();
      }
      already_found = proc.get();
    }
  }
  return ret;
}

absl::StatusOr<uint64_t> XlsccTestBase::GetBitsForChannelNameContains(
    std::string_view name_cont) {
  XLS_CHECK_NE(nullptr, package_.get());
  uint64_t ret = 0;

  const xls::Channel* already_found = nullptr;
  for (const xls::Channel* channel : package_->channels()) {
    if (absl::StrContains(channel->name(), name_cont)) {
      if (already_found != nullptr) {
        return absl::NotFoundError(absl::StrFormat(
            "Channel with name containing %s already found, %s vs %s",
            name_cont, already_found->name(), channel->name()));
      }

      ret = channel->type()->GetFlatBitCount();
      already_found = channel;
    }
  }
  return ret;
}

absl::StatusOr<xlscc_metadata::MetadataOutput>
XlsccTestBase::GenerateMetadata() {
  return translator_->GenerateMetadata();
}

absl::StatusOr<xlscc::HLSBlock> XlsccTestBase::GetBlockSpec() {
  return block_spec_;
}

absl::StatusOr<std::vector<xls::Node*>> XlsccTestBase::GetIOOpsForChannel(
    xls::FunctionBase* proc, std::string_view channel) {
  std::vector<xls::Node*> ret;
  for (xls::Node* node : proc->nodes()) {
    if (node->Is<xls::Send>()) {
      if (node->As<xls::Send>()->channel_name() == channel) {
        ret.push_back(node);
      }
    }
    if (node->Is<xls::Receive>()) {
      if (node->As<xls::Receive>()->channel_name() == channel) {
        ret.push_back(node);
      }
    }
  }
  return ret;
}

absl::Status XlsccTestBase::TokensForNode(
    xls::Node* node, absl::flat_hash_set<xls::Node*>& predecessors) {
  if (node->Is<xls::Send>()) {
    predecessors.insert(node->As<xls::Send>()->token());
    return absl::OkStatus();
  }
  if (node->Is<xls::Receive>()) {
    predecessors.insert(node->As<xls::Receive>()->token());
    return absl::OkStatus();
  }
  if (node->Is<xls::TupleIndex>()) {
    predecessors.insert(node->As<xls::TupleIndex>()->operand(0));
    return absl::OkStatus();
  }
  if (node->Is<xls::AfterAll>()) {
    for (xls::Node* operand : node->As<xls::AfterAll>()->operands()) {
      predecessors.insert(operand);
    }
    return absl::OkStatus();
  }
  return absl::UnimplementedError(absl::StrFormat(
      "Don't know how to get token for node %s", node->ToString()));
}

absl::StatusOr<bool> XlsccTestBase::NodeIsAfterTokenWise(xls::Proc* proc,
                                                         xls::Node* before,
                                                         xls::Node* after) {
  absl::flat_hash_set<xls::Node*> tokens_after = {after};

  while (!tokens_after.contains(proc->TokenParam())) {
    // Don't change the set while iterating through it
    absl::flat_hash_set<xls::Node*> next_tokens_after = {};
    for (xls::Node* node_after : tokens_after) {
      XLS_RETURN_IF_ERROR(TokensForNode(node_after, next_tokens_after));
    }
    tokens_after = next_tokens_after;

    if (tokens_after.contains(before)) {
      return true;
    }
  }

  return false;
}

absl::StatusOr<std::vector<xls::Node*>> XlsccTestBase::GetOpsForChannel(
    std::string_view channel) {
  std::vector<xls::Node*> ret;
  XLS_CHECK_NE(package_.get(), nullptr);
  for (const std::unique_ptr<xls::Proc>& proc : package_->procs()) {
    for (xls::Node* node : proc->nodes()) {
      if (node->Is<xls::Receive>() &&
          node->As<xls::Receive>()->channel_name() == channel) {
        ret.push_back(node);
      }
      if (node->Is<xls::Send>() &&
          node->As<xls::Send>()->channel_name() == channel) {
        ret.push_back(node);
      }
    }
  }
  return ret;
}

void XlsccTestBase::IOTest(std::string_view content, std::list<IOOpTest> inputs,
                           std::list<IOOpTest> outputs,
                           absl::flat_hash_map<std::string, xls::Value> args) {
  xlscc::GeneratedFunction* func;
  XLS_ASSERT_OK_AND_ASSIGN(std::string ir_src,
                           SourceToIr(content, &func, /* clang_argv= */ {},
                                      /* io_test_mode= */ true));

  XLS_LOG(INFO) << "Package IR: ";
  XLS_LOG(INFO) << ir_src;

  XLS_ASSERT_OK_AND_ASSIGN(package_, ParsePackage(ir_src));

  XLS_ASSERT_OK_AND_ASSIGN(xls::Function * entry, package_->GetTopAsFunction());

  int64_t io_ops_values = 0;
  for (const xlscc::IOOp& op : func->io_ops) {
    if (op.op == xlscc::OpType::kRead) {
      io_ops_values += 2;
    } else {
      ++io_ops_values;
    }
  }

  const int total_test_ops = inputs.size() + outputs.size();
  ASSERT_EQ(io_ops_values, total_test_ops);

  std::list<IOOpTest> input_ops_orig = inputs;
  for (const xlscc::IOOp& op : func->io_ops) {
    std::string ch_name;

    if (op.op == xlscc::OpType::kTrace) {
      continue;
    }

    ch_name = op.channel->unique_name;

    const std::string arg_name =
        absl::StrFormat("%s_op%i", ch_name, op.channel_op_index);

    if (op.op == xlscc::OpType::kRecv || op.op == xlscc::OpType::kRead) {
      const IOOpTest test_op = inputs.front();
      inputs.pop_front();

      std::string expected_name = ch_name;
      if (op.op == xlscc::OpType::kRead) {
        expected_name += "__read";
      }
      XLS_CHECK_EQ(expected_name, test_op.name);

      const xls::Value& new_val = test_op.value;

      if (!args.contains(arg_name)) {
        args[arg_name] = new_val;
        continue;
      }

      if (args[arg_name].IsBits()) {
        args[arg_name] = xls::Value::Tuple({args[arg_name], new_val});
      } else {
        XLS_CHECK(args[arg_name].IsTuple());
        const xls::Value prev_val = args[arg_name];
        XLS_ASSERT_OK_AND_ASSIGN(std::vector<xls::Value> values,
                                 prev_val.GetElements());
        values.push_back(new_val);
        args[arg_name] = xls::Value::Tuple(values);
      }
    }
  }

  XLS_ASSERT_OK_AND_ASSIGN(
      xls::Value actual,
      DropInterpreterEvents(xls::InterpretFunctionKwargs(entry, args)));

  std::vector<xls::Value> returns;

  if (total_test_ops > 1) {
    ASSERT_TRUE(actual.IsTuple());
    XLS_ASSERT_OK_AND_ASSIGN(returns, actual.GetElements());
  } else {
    returns.push_back(actual);
  }

  // Every op at least returns a condition
  ASSERT_EQ(returns.size(), func->io_ops.size());

  inputs = input_ops_orig;

  int op_idx = 0;
  for (const xlscc::IOOp& op : func->io_ops) {
    std::string ch_name;

    if (op.op == xlscc::OpType::kTrace) {
      ch_name = "__trace";
    } else {
      ch_name = op.channel->unique_name;
    }

    if (op.op == xlscc::OpType::kRecv || op.op == xlscc::OpType::kRead) {
      const IOOpTest test_op = inputs.front();
      inputs.pop_front();

      std::string expected_name = ch_name;
      if (op.op == xlscc::OpType::kRead) {
        expected_name += "__read";
      }
      XLS_CHECK_EQ(expected_name, test_op.name);

      xls::Value cond_val;

      if (op.op == xlscc::OpType::kRecv) {
        cond_val = returns[op_idx];
      } else {
        ASSERT_TRUE(returns[op_idx].IsTuple());
        XLS_ASSERT_OK_AND_ASSIGN(std::vector<xls::Value> elements,
                                 returns[op_idx].GetElements());
        ASSERT_EQ(elements.size(), 2);
        cond_val = elements[1];
        // Check address value if condition is true
        XLS_ASSERT_OK_AND_ASSIGN(uint64_t cond_output,
                                 cond_val.bits().ToUint64());
        const IOOpTest addr_op = outputs.front();
        outputs.pop_front();
        EXPECT_EQ(addr_op.condition ? 1 : 0, cond_output);
        if (cond_output == 1u) {
          EXPECT_EQ(elements[0], addr_op.value);
        }
      }

      ASSERT_TRUE(cond_val.IsBits());
      XLS_ASSERT_OK_AND_ASSIGN(uint64_t val, cond_val.bits().ToUint64());
      EXPECT_EQ(val, test_op.condition ? 1 : 0);

    } else if (op.op == xlscc::OpType::kSend ||
               op.op == xlscc::OpType::kWrite) {
      const IOOpTest test_op = outputs.front();
      outputs.pop_front();

      std::string expected_name = ch_name;
      if (op.op == xlscc::OpType::kWrite) {
        expected_name += "__write";
      }

      XLS_CHECK_EQ(expected_name, test_op.name);

      ASSERT_TRUE(returns[op_idx].IsTuple());
      XLS_ASSERT_OK_AND_ASSIGN(std::vector<xls::Value> elements,
                               returns[op_idx].GetElements());
      ASSERT_EQ(elements.size(), 2);
      ASSERT_TRUE(elements[1].IsBits());
      XLS_ASSERT_OK_AND_ASSIGN(uint64_t cond_output,
                               elements[1].bits().ToUint64());
      EXPECT_EQ(cond_output, test_op.condition ? 1 : 0);
      // Don't check data if it wasn't sent
      if (cond_output != 0u) {
        EXPECT_EQ(elements[0], test_op.value);
      }
    } else if (op.op == xlscc::OpType::kTrace) {
      const IOOpTest test_op = outputs.front();
      outputs.pop_front();

      XLS_CHECK_EQ(ch_name, test_op.name);
      EXPECT_EQ(test_op.message, op.trace_message_string);
      EXPECT_EQ(test_op.label, op.label_string);
      EXPECT_EQ(test_op.trace_type, op.trace_type);

      // No conditions for traces
      EXPECT_TRUE(test_op.condition);

      EXPECT_EQ(returns[op_idx], test_op.value);
    } else {
      FAIL() << "IOOp of unknown type: " << static_cast<int>(op.op);
    }
    ++op_idx;
  }

  ASSERT_EQ(inputs.size(), 0);
  ASSERT_EQ(outputs.size(), 0);
}

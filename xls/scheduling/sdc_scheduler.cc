// Copyright 2022 The XLS Authors
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

#include "xls/scheduling/sdc_scheduler.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/types/span.h"
#include "xls/common/logging/log_lines.h"
#include "xls/common/logging/logging.h"
#include "xls/common/logging/vlog_is_on.h"
#include "xls/common/status/ret_check.h"
#include "xls/common/status/status_macros.h"
#include "xls/delay_model/delay_estimator.h"
#include "xls/ir/channel.h"
#include "xls/ir/function.h"
#include "xls/ir/function_base.h"
#include "xls/ir/node.h"
#include "xls/ir/node_iterator.h"
#include "xls/ir/node_util.h"
#include "xls/ir/nodes.h"
#include "xls/ir/op.h"
#include "xls/ir/proc.h"
#include "xls/scheduling/scheduling_options.h"
#include "ortools/math_opt/cpp/math_opt.h"

namespace xls {

namespace {

using DelayMap = absl::flat_hash_map<Node*, int64_t>;
namespace math_opt = ::operations_research::math_opt;

// A helper function to compute each node's delay by calling the delay estimator
absl::StatusOr<DelayMap> ComputeNodeDelays(
    FunctionBase* f, const DelayEstimator& delay_estimator) {
  DelayMap result;
  for (Node* node : f->nodes()) {
    XLS_ASSIGN_OR_RETURN(result[node],
                         delay_estimator.GetOperationDelayInPs(node));
  }
  return result;
}

// Compute all-pairs longest distance between all nodes in `f`. The distance
// from node `a` to node `b` is defined as the length of the longest delay path
// from `a`'s start to `b`'s end, which includes the delay of the path endpoints
// `a` and `b`. The all-pairs distance is stored in the map of maps
// `distances_to_node` where `distances_to_node[y][x]` (if present) is the
// critical-path distance from `x` to `y`.
absl::flat_hash_map<Node*, absl::flat_hash_map<Node*, int64_t>>
ComputeDistancesToNodes(FunctionBase* f, const NodeIterator& topo_sort,
                        const DelayMap& delay_map) {
  absl::flat_hash_map<Node*, absl::flat_hash_map<Node*, int64_t>>
      distances_to_node;
  distances_to_node.reserve(f->node_count());
  for (Node* node : topo_sort) {
    // Initialize the distance map entry to an empty map.
    distances_to_node[node];
  }

  for (Node* node : topo_sort) {
    absl::flat_hash_map<Node*, int64_t>& distances = distances_to_node.at(node);

    // The critical path from `node` to `node` is always `node_delay` long.
    int64_t node_delay = delay_map.at(node);
    distances_to_node.at(node)[node] = node_delay;

    // Compute the critical-path distance from `a` to `node` for all descendants
    // `a` of each operand, extending the critical path from `a` to each operand
    // of `node` by `node_delay`.
    for (Node* operand : node->operands()) {
      for (auto [a, operand_distance] : distances_to_node.at(operand)) {
        auto [it, newly_reachable] =
            distances.try_emplace(a, operand_distance + node_delay);
        if (!newly_reachable) {
          if (it->second >= operand_distance + node_delay) {
            continue;
          }
          it->second = operand_distance + node_delay;
        }
      }
    }
  }

  if (XLS_VLOG_IS_ON(4)) {
    XLS_VLOG(4) << "All-pairs critical-path distances:";
    for (Node* target : topo_sort) {
      XLS_VLOG(4) << absl::StrFormat("  distances to %s:", target->GetName());
      for (Node* source : topo_sort) {
        XLS_VLOG(4) << absl::StrFormat(
            "    %s -> %s : %s", source->GetName(), target->GetName(),
            distances_to_node.at(target).contains(source)
                ? absl::StrCat(distances_to_node.at(target).at(source))
                : "(none)");
      }
    }
  }

  return distances_to_node;
}

// Returns the minimal set of schedule constraints which ensure that no
// combinational path in the schedule exceeds `clock_period_ps`. The returned
// map has a (potentially empty) vector entry for each node in `f`. The map
// value (vector of nodes) for node `x` is the set of nodes which must be
// scheduled at least one cycle later than `x`. That is, if `return_value[x]` is
// `S` then:
//
//   cycle(i) + 1 >= cycle(x) for i \in S
//
// The set of constraints is a minimal set which guarantees that no
// combinational path violates the clock period timing. Specifically, `(a, b)`
// is in the set of returned constraints (ie., `return_value[a]` contains `b`)
// iff critical-path distance from `a` to `b` including the delay of `a` and `b`
// is greater than `critical_path_period`, but the critical-path distance of the
// path *not* including the delay of `b` is *less than* `critical_path_period`.
absl::flat_hash_map<Node*, std::vector<Node*>>
ComputeCombinationalDelayConstraints(
    FunctionBase* f, const NodeIterator& topo_sort, int64_t clock_period_ps,
    const absl::flat_hash_map<Node*, absl::flat_hash_map<Node*, int64_t>>&
        distances_to_node,
    const DelayMap& delay_map) {
  absl::flat_hash_map<Node*, std::vector<Node*>> result;
  result.reserve(f->node_count());
  for (Node* a : topo_sort) {
    result[a];
  }

  for (Node* node : topo_sort) {
    const int64_t node_delay = delay_map.at(node);

    // For each ancestor `a`, check whether the critical-path length from `a`'s
    // start to `node`'s end crosses a `clock_period_ps` boundary due to
    // `node`'s delay. If so, we need a constraint to ensure that `node` is in a
    // later stage than `a`.
    //
    // NOTE: The order in which we iterate over the ancestors `a` here does not
    // matter. As long as our iteration over `node` is deterministic, we will
    // push the same sequence of `node`s into each `result[a]` every time.
    for (auto [a, distance] : distances_to_node.at(node)) {
      if (distance > clock_period_ps &&
          distance - node_delay <= clock_period_ps) {
        result.at(a).push_back(node);
      }
    }
  }

  if (XLS_VLOG_IS_ON(4)) {
    XLS_VLOG(4) << absl::StrFormat("Constraints (clock period: %dps):",
                                   clock_period_ps);
    for (Node* node : topo_sort) {
      XLS_VLOG(4) << absl::StrFormat("  %s: [%s]", node->GetName(),
                                     absl::StrJoin(result.at(node), ", "));
    }
  }
  return result;
}

}  // namespace

SDCSchedulingModel::SDCSchedulingModel(FunctionBase* func,
                                       const DelayMap& delay_map,
                                       std::string_view model_name)
    : func_(func),
      topo_sort_(TopoSort(func_)),
      model_(model_name),
      delay_map_(delay_map),
      last_stage_(model_.AddContinuousVariable(0.0, kInfinity, "last_stage")),
      cycle_at_sinknode_(model_.AddContinuousVariable(-kInfinity, kInfinity,
                                                      "cycle_at_sinknode")) {
  // when subclassed for Iterative SDC, delay_map_ and distances_to_node_
  // are not used.
  if (!delay_map_.empty()) {
    distances_to_node_ = ComputeDistancesToNodes(func_, topo_sort_, delay_map_);
  }

  for (Node* node : topo_sort_) {
    cycle_var_.emplace(
        node, model_.AddContinuousVariable(0.0, kInfinity, node->GetName()));
    model_.AddLinearConstraint(
        cycle_var_.at(node) <= last_stage_,
        absl::StrFormat("pipeline_length:%s", node->GetName()));
    lifetime_var_.emplace(
        node,
        model_.AddContinuousVariable(
            0.0, kInfinity, absl::StrFormat("lifetime_%s", node->GetName())));
  }

  if (func_->IsFunction()) {
    Function* function = func_->AsFunctionOrDie();
    // For functions, all parameter nodes must be scheduled in the first stage
    // of the pipeline...
    for (Param* param : function->params()) {
      model_.AddLinearConstraint(cycle_var_.at(param) <= 0,
                                 absl::StrCat("param:", param->GetName()));
    }

    // ... and the return value must be scheduled in the final stage, unless
    // it's a parameter.
    if (!function->return_value()->Is<Param>()) {
      model_.AddLinearConstraint(
          cycle_var_.at(function->return_value()) >= last_stage_,
          absl::StrCat("return:", function->return_value()->GetName()));
    }
  }
}

absl::Status SDCSchedulingModel::AddDefUseConstraints(
    Node* node, std::optional<Node*> user) {
  XLS_RETURN_IF_ERROR(AddCausalConstraint(node, user));
  XLS_RETURN_IF_ERROR(AddLifetimeConstraint(node, user));
  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddCausalConstraint(
    Node* node, std::optional<Node*> user) {
  math_opt::Variable cycle_at_node = cycle_var_.at(node);
  math_opt::Variable cycle_at_user =
      user.has_value() ? cycle_var_.at(user.value()) : cycle_at_sinknode_;

  // Explicit delay nodes must lag their inputs by a certain number of cycles.
  int64_t min_delay = 0;
  if (user.has_value() && user.value()->Is<MinDelay>()) {
    min_delay = user.value()->As<MinDelay>()->delay();
  }

  std::string user_str = user.has_value() ? user.value()->GetName() : "«sink»";

  model_.AddLinearConstraint(
      cycle_at_user - cycle_at_node >= static_cast<double>(min_delay),
      absl::StrFormat("causal_%s_%s", node->GetName(), user_str));
  XLS_VLOG(2) << "Setting causal constraint: "
              << absl::StrFormat("cycle[%s] - cycle[%s] ≥ %d", user_str,
                                 node->GetName(), min_delay);

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddLifetimeConstraint(
    Node* node, std::optional<Node*> user) {
  math_opt::Variable cycle_at_node = cycle_var_.at(node);
  math_opt::Variable lifetime_at_node = lifetime_var_.at(node);
  math_opt::Variable cycle_at_user =
      user.has_value() ? cycle_var_.at(user.value()) : cycle_at_sinknode_;

  std::string user_str = user.has_value() ? user.value()->GetName() : "«sink»";

  model_.AddLinearConstraint(
      lifetime_at_node + cycle_at_node - cycle_at_user >= 0,
      absl::StrFormat("lifetime_%s_%s", node->GetName(), user_str));
  XLS_VLOG(2) << "Setting lifetime constraint: "
              << absl::StrFormat("lifetime[%s] + cycle[%s] - cycle[%s] ≥ 0",
                                 node->GetName(), node->GetName(), user_str);

  return absl::OkStatus();
}

// This ensures that state backedges don't span more than II cycles, which is
// necessary while enforcing a target II.
absl::Status SDCSchedulingModel::AddBackedgeConstraints(
    const BackedgeConstraint& constraint) {
  Proc* proc = dynamic_cast<Proc*>(func_);
  if (proc == nullptr) {
    return absl::OkStatus();
  }
  const int64_t II = proc->GetInitiationInterval().value_or(1);

  using StateIndex = int64_t;
  for (StateIndex i = 0; i < proc->GetStateElementCount(); ++i) {
    Node* const state = proc->GetStateParam(i);
    Node* const next = proc->GetNextStateElement(i);
    if (next == state) {
      continue;
    }
    XLS_VLOG(2) << "Setting backedge constraint (II): "
                << absl::StrFormat("cycle[%s] - cycle[%s] < %d",
                                   next->GetName(), state->GetName(), II);
    backedge_constraint_.emplace(
        std::make_pair(state, next),
        DiffLessThanConstraint(next, state, II, "backedge"));
  }

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddSchedulingConstraint(
    const SchedulingConstraint& constraint) {
  if (std::holds_alternative<BackedgeConstraint>(constraint)) {
    return AddBackedgeConstraints(std::get<BackedgeConstraint>(constraint));
  }
  if (std::holds_alternative<IOConstraint>(constraint)) {
    return AddIOConstraint(std::get<IOConstraint>(constraint));
  }
  if (std::holds_alternative<NodeInCycleConstraint>(constraint)) {
    return AddNodeInCycleConstraint(
        std::get<NodeInCycleConstraint>(constraint));
  }
  if (std::holds_alternative<DifferenceConstraint>(constraint)) {
    return AddDifferenceConstraint(std::get<DifferenceConstraint>(constraint));
  }
  if (std::holds_alternative<RecvsFirstSendsLastConstraint>(constraint)) {
    return AddRFSLConstraint(
        std::get<RecvsFirstSendsLastConstraint>(constraint));
  }
  if (std::holds_alternative<SendThenRecvConstraint>(constraint)) {
    return AddSendThenRecvConstraint(
        std::get<SendThenRecvConstraint>(constraint));
  }
  return absl::InternalError("Unhandled scheduling constraint type");
}

absl::Status SDCSchedulingModel::AddIOConstraint(
    const IOConstraint& constraint) {
  // Map from channel name to set of nodes that send/receive on that channel.
  absl::flat_hash_map<std::string, std::vector<Node*>> channel_to_nodes;
  for (Node* node : topo_sort_) {
    if (node->Is<Receive>() || node->Is<Send>()) {
      XLS_ASSIGN_OR_RETURN(Channel * channel, GetChannelUsedByNode(node));
      channel_to_nodes[channel->name()].push_back(node);
    }
  }

  // We use `channel_to_nodes[...]` instead of `channel_to_nodes.at(...)`
  // below because we don't want to error out if a constraint is specified
  // that affects a channel with no associated send/receives in this proc.
  for (Node* source : channel_to_nodes[constraint.SourceChannel()]) {
    for (Node* target : channel_to_nodes[constraint.TargetChannel()]) {
      auto node_matches_direction = [](Node* node, IODirection dir) -> bool {
        return (node->Is<Send>() && dir == IODirection::kSend) ||
               (node->Is<Receive>() && dir == IODirection::kReceive);
      };
      if (!node_matches_direction(source, constraint.SourceDirection())) {
        continue;
      }
      if (!node_matches_direction(target, constraint.TargetDirection())) {
        continue;
      }
      if (source == target) {
        continue;
      }

      XLS_VLOG(2) << "Setting IO constraint: "
                  << absl::StrFormat("%d ≤ cycle[%s] - cycle[%s] ≤ %d",
                                     constraint.MinimumLatency(),
                                     target->GetName(), source->GetName(),
                                     constraint.MaximumLatency());
      io_constraints_[constraint].push_back({
          .lower = DiffAtLeastConstraint(target, source,
                                         constraint.MinimumLatency(), "io"),
          .upper = DiffAtMostConstraint(target, source,
                                        constraint.MaximumLatency(), "io"),
      });
    }
  }

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddNodeInCycleConstraint(
    const NodeInCycleConstraint& constraint) {
  Node* node = constraint.GetNode();
  int64_t cycle = constraint.GetCycle();

  model_.AddLinearConstraint(cycle_var_.at(node) == static_cast<double>(cycle),
                             absl::StrFormat("nic_%s", node->GetName()));
  XLS_VLOG(2) << "Setting node-in-cycle constraint: "
              << absl::StrFormat("cycle[%s] = %d", node->GetName(), cycle);

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddDifferenceConstraint(
    const DifferenceConstraint& constraint) {
  Node* a = constraint.GetA();
  Node* b = constraint.GetB();
  int64_t max_difference = constraint.GetMaxDifference();
  DiffAtMostConstraint(a, b, max_difference, "diff");

  XLS_VLOG(2) << "Setting difference constraint: "
              << absl::StrFormat("cycle[%s] - cycle[%s] ≤ %d", a->GetName(),
                                 b->GetName(), max_difference);

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddRFSLConstraint(
    const RecvsFirstSendsLastConstraint& constraint) {
  for (Node* node : topo_sort_) {
    if (node->Is<Receive>()) {
      XLS_VLOG(2) << "Setting receive-in-first-cycle constraint: "
                  << absl::StrFormat("cycle[%s] ≤ 0", node->GetName());
      model_.AddLinearConstraint(cycle_var_.at(node) <= 0,
                                 absl::StrFormat("recv_%s", node->GetName()));
    } else if (node->Is<Send>()) {
      XLS_VLOG(2) << "Setting send-in-last-cycle constraint: "
                  << absl::StrFormat("%s ≤ cycle[%s]", last_stage_.name(),
                                     node->GetName());
      model_.AddLinearConstraint(cycle_var_.at(node) >= last_stage_,
                                 absl::StrFormat("send_%s", node->GetName()));
    }
  }

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::AddSendThenRecvConstraint(
    const SendThenRecvConstraint& constraint) {
  XLS_CHECK_GE(constraint.MinimumLatency(), 0);
  if (constraint.MinimumLatency() == 0) {
    return absl::OkStatus();
  }

  for (Node* recv : topo_sort_) {
    if (!recv->Is<Receive>()) {
      continue;
    }

    // Look for a Send dependency.
    //
    // Technically, we probably don't need to trace back through the predicate
    // operand; the only operation we have today that takes a token and returns
    // data is a Receive (and technically tuple construction, but that just gets
    // weird), so we'd end up terminating our search before reaching a Send
    // anyway. But - just in case we ever add some other operation, we'll trace
    // up both paths to be sure.
    std::vector<Node*> stack(recv->operands().begin(), recv->operands().end());
    absl::flat_hash_set<Node*> seen;
    while (!stack.empty()) {
      Node* node = stack.back();
      stack.pop_back();
      if (seen.contains(node)) {
        continue;
      }
      seen.insert(node);

      if (node->Is<Send>()) {
        // Ensure that this send happens before the receive that depends on it.
        DiffAtLeastConstraint(recv, node, constraint.MinimumLatency(),
                              "send_then_recv");
        // We don't need to trace any further back on this line, since any
        // earlier sends are transitively handled.
        continue;
      }
      if (node->Is<Receive>()) {
        // No need to trace any further back on this line; this node will also
        // be the root of a search, and will get a similar appropriate delay.
        continue;
      }
      stack.insert(stack.end(), node->operands().begin(),
                   node->operands().end());
    }
  }
  return absl::OkStatus();
}

void SDCSchedulingModel::SetObjective() {
  math_opt::LinearExpression objective;
  for (Node* node : topo_sort_) {
    // Minimize node lifetimes.
    // The scaling makes the tie-breaker small in comparison, and is a power
    // of two so that there's no imprecision (just add to exponent).
    objective += 1024 *
                 static_cast<double>(node->GetType()->GetFlatBitCount()) *
                 lifetime_var_.at(node);
    // This acts as a tie-breaker for under-constrained problems, favoring ASAP
    // schedules.
    objective += cycle_var_.at(node);
  }
  model_.Minimize(objective);
}

void SDCSchedulingModel::RemoveObjective() { model_.Minimize(0.0); }

absl::StatusOr<ScheduleCycleMap> SDCSchedulingModel::ExtractResult(
    const math_opt::VariableMap<double>& variable_values) const {
  ScheduleCycleMap cycle_map;
  for (Node* node : topo_sort_) {
    double cycle = variable_values.at(cycle_var_.at(node));
    if (std::fabs(cycle - std::round(cycle)) > 0.001) {
      return absl::InternalError(
          "The scheduling result is expected to be integer");
    }
    cycle_map[node] = std::round(cycle);
  }
  return cycle_map;
}

void SDCSchedulingModel::SetClockPeriod(int64_t clock_period_ps) {
  absl::flat_hash_map<Node*, std::vector<Node*>> prev_delay_constraints =
      std::move(delay_constraints_);
  delay_constraints_ = ComputeCombinationalDelayConstraints(
      func_, topo_sort_, clock_period_ps, distances_to_node_, delay_map_);

  for (Node* source : topo_sort_) {
    if (!prev_delay_constraints.empty()) {
      // Check over all the prior constraints, dropping any that are obsolete.
      absl::flat_hash_set<Node*> new_targets(
          delay_constraints_.at(source).begin(),
          delay_constraints_.at(source).end());
      for (Node* target : prev_delay_constraints.at(source)) {
        if (new_targets.contains(target)) {
          continue;
        }

        // No longer related; remove constraint.
        auto it = timing_constraint_.find(std::make_pair(source, target));
        model_.DeleteLinearConstraint(it->second);
        timing_constraint_.erase(it);
      }
    }

    // Add all new constraints, avoiding duplicates for any that already exist.
    for (Node* target : delay_constraints_.at(source)) {
      auto key = std::make_pair(source, target);
      if (timing_constraint_.contains(key)) {
        continue;
      }

      // Newly related; add constraint.
      XLS_VLOG(2) << "Setting timing constraint: "
                  << absl::StrFormat("1 ≤ %s - %s", target->GetName(),
                                     source->GetName());
      timing_constraint_.emplace(
          key, DiffAtLeastConstraint(target, source, 1, "timing"));
    }
  }
}

void SDCSchedulingModel::SetPipelineLength(
    std::optional<int64_t> pipeline_length) {
  if (pipeline_length.has_value()) {
    model_.set_lower_bound(last_stage_,
                           static_cast<double>(*pipeline_length - 1));
    model_.set_upper_bound(last_stage_,
                           static_cast<double>(*pipeline_length - 1));
  } else {
    model_.set_lower_bound(last_stage_, 0.0);
    model_.set_upper_bound(last_stage_, kInfinity);
  }
}

void SDCSchedulingModel::MinimizePipelineLength() {
  model_.Minimize(last_stage_);
}

absl::StatusOr<int64_t> SDCSchedulingModel::ExtractPipelineLength(
    const operations_research::math_opt::VariableMap<double>& variable_values)
    const {
  double last_stage = variable_values.at(last_stage_);
  if (std::fabs(last_stage - std::round(last_stage)) > 0.001) {
    return absl::InternalError(absl::StrCat(
        "The optimum pipeline length is expected to be an integer, was: ",
        last_stage + 1.0));
  }
  return static_cast<int64_t>(std::round(last_stage)) + 1;
}

absl::Status SDCSchedulingModel::AddSlackVariables(
    std::optional<double> infeasible_per_state_backedge_slack_pool) {
  if (infeasible_per_state_backedge_slack_pool.has_value()) {
    XLS_RET_CHECK_GT(*infeasible_per_state_backedge_slack_pool, 0)
        << "infeasible_per_state_backedge_slack_pool must be positive";
  }
  // Add slack variables to all relevant constraints.

  // First, remove the upper bound on pipeline length, but try to minimize it
  // (dropping any other objective we have). We assume users are most willing to
  // relax this; i.e., they care about throughput more than latency.
  model_.set_upper_bound(last_stage_, kInfinity);
  model_.Minimize(last_stage_);

  // Next, if this is a proc, relax the state back-edge length restriction (if
  // present). We assume users are reasonably willing to relax this; i.e., they
  // care about throughput, but they care more about the I/O constraints they've
  // specified.
  if (Proc* proc = dynamic_cast<Proc*>(func_);
      proc != nullptr && !backedge_constraint_.empty()) {
    double backedge_slack_objective_scale = static_cast<double>(1 << 10);
    shared_backedge_slack_ = model_.AddVariable(
        0.0, kInfinity, /*is_integer=*/false, "backedge_slack");
    model_.AddToObjective(
        (backedge_slack_objective_scale * shared_backedge_slack_.value()));
    for (auto& [nodes, constraint] : backedge_constraint_) {
      AddUpperBoundSlack(constraint, shared_backedge_slack_);
      if (infeasible_per_state_backedge_slack_pool.has_value()) {
        auto [itr, inserted] = node_backedge_slack_.try_emplace(
            nodes,
            model_.AddVariable(0.0, kInfinity, /*is_integer=*/false,
                               absl::StrFormat("%v_to_%v_backedge_slack",
                                               *nodes.first, *nodes.second)));
        XLS_RET_CHECK(inserted);
        operations_research::math_opt::Variable& node_to_node_slack =
            itr->second;
        model_.AddToObjective(
            backedge_slack_objective_scale /
            infeasible_per_state_backedge_slack_pool.value() *
            node_to_node_slack);
        AddUpperBoundSlack(constraint, node_to_node_slack);
      }
    }
  }

  // Finally, relax the I/O constraints, if nothing else works.
  for (auto& [io_constraint, constraints] : io_constraints_) {
    math_opt::Variable min_slack = model_.AddVariable(
        0, kInfinity, /*is_integer=*/false,
        absl::StrCat("io_min_", io_constraint.SourceChannel(), "→",
                     io_constraint.TargetChannel(), "_slack"));
    math_opt::Variable max_slack = model_.AddVariable(
        0, kInfinity, /*is_integer=*/false,
        absl::StrCat("io_max_", io_constraint.SourceChannel(), "→",
                     io_constraint.TargetChannel(), "_slack"));
    model_.AddToObjective((1 << 20) * min_slack);
    model_.AddToObjective((1 << 20) * max_slack);
    io_slack_.emplace(io_constraint, SlackPair{
                                         .min = min_slack,
                                         .max = max_slack,
                                     });

    for (auto& [min_constraint, max_constraint] : constraints) {
      AddLowerBoundSlack(min_constraint, min_slack);
      AddUpperBoundSlack(max_constraint, max_slack);
    }
  }

  return absl::OkStatus();
}

absl::Status SDCSchedulingModel::ExtractError(
    const math_opt::VariableMap<double>& variable_values) const {
  std::vector<std::string> problems;
  std::vector<std::string> suggestions;
  double last_stage = variable_values.at(last_stage_);
  if (last_stage > last_stage_.lower_bound() + 0.001) {
    int64_t new_pipeline_length =
        static_cast<int64_t>(std::ceil(last_stage)) + 1;
    problems.push_back("the specified pipeline length");
    suggestions.push_back(
        absl::StrCat("`--pipeline_stages=", new_pipeline_length, "`"));
  }
  if (func_->IsProc() && shared_backedge_slack_.has_value()) {
    double backedge_slack = variable_values.at(*shared_backedge_slack_);
    if (backedge_slack > 0.001) {
      int64_t new_backedge_length =
          func_->AsProcOrDie()->GetInitiationInterval().value_or(1) +
          static_cast<int64_t>(std::ceil(backedge_slack));
      problems.push_back("full throughput");
      suggestions.push_back(
          absl::StrCat("`--worst_case_throughput=", new_backedge_length, "`"));
    }
    for (const auto& [nodes, node_backedge_var] : node_backedge_slack_) {
      double node_backedge = variable_values.at(node_backedge_var);
      if (node_backedge > 0.001) {
        problems.push_back("full throughput");
        suggestions.push_back(absl::StrFormat(
            "looking at paths between %v and %v (needs %d additional slack)",
            *nodes.first, *nodes.second,
            static_cast<int64_t>(std::ceil(node_backedge))));
      }
    }
  }
  if (!problems.empty()) {
    if (problems.size() == 1 || problems.size() == 2) {
      return absl::InvalidArgumentError(
          absl::StrCat("cannot achieve ", absl::StrJoin(problems, " or "),
                       ". Try ", absl::StrJoin(suggestions, " and ")));
    }
    return absl::InvalidArgumentError(absl::StrCat(
        "cannot achieve ",
        absl::StrJoin(
            absl::MakeConstSpan(problems).subspan(0, problems.size() - 1),
            ", "),
        " or ", problems[problems.size() - 1], ". Try ",
        absl::StrJoin(
            absl::MakeConstSpan(suggestions).subspan(0, suggestions.size() - 1),
            ", "),
        " and ", suggestions[suggestions.size() - 1]));
  }

  std::vector<std::string> io_problems;
  for (auto& [io_constraint, slacks] : io_slack_) {
    double min_slack = variable_values.at(slacks.min);
    double max_slack = variable_values.at(slacks.max);

    std::vector<std::string> latency_suggestions;
    if (min_slack > 0.001) {
      int64_t new_min_latency = io_constraint.MinimumLatency() -
                                static_cast<int64_t>(std::ceil(min_slack));
      latency_suggestions.push_back(
          absl::StrCat("minimum latency ≤ ", new_min_latency));
    }
    if (max_slack > 0.001) {
      int64_t new_max_latency = io_constraint.MaximumLatency() +
                                static_cast<int64_t>(std::ceil(max_slack));
      latency_suggestions.push_back(
          absl::StrCat("maximum latency ≥ ", new_max_latency));
    }

    if (latency_suggestions.empty()) {
      continue;
    }
    io_problems.push_back(absl::StrCat(
        io_constraint.SourceChannel(), "→", io_constraint.TargetChannel(),
        " with ", absl::StrJoin(latency_suggestions, ", ")));
  }
  if (!io_problems.empty()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "cannot satisfy the given I/O constraints. Would succeed with: ",
        absl::StrJoin(io_problems, ", ",
                      [](std::string* out, const std::string& entry) {
                        absl::StrAppend(out, "{", entry, "}");
                      })));
  }

  return absl::UnknownError("reason unknown.");
}

math_opt::LinearConstraint SDCSchedulingModel::DiffAtMostConstraint(
    Node* x, Node* y, int64_t limit, std::string_view name) {
  return model_.AddLinearConstraint(
      cycle_var_.at(x) - cycle_var_.at(y) <= static_cast<double>(limit),
      absl::StrFormat("%s:%s-%s≤%d", name, x->GetName(), y->GetName(), limit));
}

math_opt::LinearConstraint SDCSchedulingModel::DiffLessThanConstraint(
    Node* x, Node* y, int64_t limit, std::string_view name) {
  return model_.AddLinearConstraint(
      cycle_var_.at(x) - cycle_var_.at(y) <= static_cast<double>(limit - 1),
      absl::StrFormat("%s:%s-%s<%d", name, x->GetName(), y->GetName(), limit));
}

math_opt::LinearConstraint SDCSchedulingModel::DiffAtLeastConstraint(
    Node* x, Node* y, int64_t limit, std::string_view name) {
  return model_.AddLinearConstraint(
      cycle_var_.at(x) - cycle_var_.at(y) >= static_cast<double>(limit),
      absl::StrFormat("%s:%s-%s≥%d", name, x->GetName(), y->GetName(), limit));
}

math_opt::LinearConstraint SDCSchedulingModel::DiffGreaterThanConstraint(
    Node* x, Node* y, int64_t limit, std::string_view name) {
  return model_.AddLinearConstraint(
      cycle_var_.at(x) - cycle_var_.at(y) >= static_cast<double>(limit + 1),
      absl::StrFormat("%s:%s-%s≥%d", name, x->GetName(), y->GetName(), limit));
}

math_opt::LinearConstraint SDCSchedulingModel::DiffEqualsConstraint(
    Node* x, Node* y, int64_t diff, std::string_view name) {
  if (x == y) {
    XLS_LOG(FATAL) << "DiffEqualsConstraint: " << x->GetName() << " - "
                   << y->GetName() << " = " << diff << " is unsatisfiable";
  }
  return model_.AddLinearConstraint(
      cycle_var_.at(x) - cycle_var_.at(y) == static_cast<double>(diff),
      absl::StrFormat("%s:%s-%s=%d", name, x->GetName(), y->GetName(), diff));
}

math_opt::Variable SDCSchedulingModel::AddUpperBoundSlack(
    math_opt::LinearConstraint c, std::optional<math_opt::Variable> slack) {
  XLS_CHECK_LT(c.upper_bound(), kInfinity)
      << "The constraint " << c.name() << " has no upper bound.";
  if (slack.has_value()) {
    XLS_CHECK_EQ(c.coefficient(*slack), 0.0)
        << "The slack variable " << slack->name()
        << " is already referenced in the constraint " << c.name() << ".";
  } else {
    slack = model_.AddVariable(0.0, kInfinity, /*is_integer=*/false,
                               absl::StrCat(c.name(), "_ub_slack"));
  }
  model_.set_coefficient(c, *slack, -1.0);
  return *slack;
}

absl::Status SDCSchedulingModel::RemoveUpperBoundSlack(
    math_opt::Variable v, math_opt::LinearConstraint upper_bound_with_slack,
    math_opt::Variable slack) {
  XLS_RET_CHECK_EQ(upper_bound_with_slack.coefficient(v), 1.0);
  XLS_RET_CHECK_EQ(upper_bound_with_slack.coefficient(slack), -1.0);
  model_.set_upper_bound(v, upper_bound_with_slack.upper_bound());
  model_.DeleteLinearConstraint(upper_bound_with_slack);
  return absl::OkStatus();
}

math_opt::Variable SDCSchedulingModel::AddLowerBoundSlack(
    math_opt::LinearConstraint c, std::optional<math_opt::Variable> slack) {
  XLS_CHECK_GT(c.lower_bound(), -kInfinity)
      << "The constraint " << c.name() << " has no lower bound.";
  if (slack.has_value()) {
    XLS_CHECK_EQ(c.coefficient(*slack), 0.0)
        << "The slack variable " << slack->name()
        << " is already referenced in the constraint " << c.name() << ".";
  } else {
    slack = model_.AddVariable(0.0, kInfinity, /*is_integer=*/false,
                               absl::StrCat(c.name(), "_lb_slack"));
  }
  model_.set_coefficient(c, *slack, 1.0);
  return *slack;
}

std::pair<math_opt::Variable, math_opt::LinearConstraint>
SDCSchedulingModel::AddUpperBoundSlack(
    math_opt::Variable v, std::optional<math_opt::Variable> slack) {
  XLS_CHECK_LT(v.upper_bound(), kInfinity)
      << "The variable " << v.name() << " has no fixed upper bound.";
  if (!slack.has_value()) {
    slack = model_.AddVariable(0.0, kInfinity, /*is_integer=*/false,
                               absl::StrCat(v.name(), "_ub_slack"));
  }
  math_opt::LinearConstraint upper_bound = model_.AddLinearConstraint(
      v - *slack <= v.upper_bound(), absl::StrCat(v.name(), "_ub"));
  model_.set_upper_bound(v, kInfinity);
  return {*slack, upper_bound};
}

std::pair<math_opt::Variable, math_opt::LinearConstraint>
SDCSchedulingModel::AddLowerBoundSlack(
    math_opt::Variable v, std::optional<math_opt::Variable> slack) {
  XLS_CHECK_GT(v.lower_bound(), -kInfinity)
      << "The variable " << v.name() << " has no fixed lower bound.";
  if (!slack.has_value()) {
    slack = model_.AddVariable(0.0, kInfinity, /*is_integer=*/false,
                               absl::StrCat(v.name(), "_lb_slack"));
  }
  math_opt::LinearConstraint lower_bound = model_.AddLinearConstraint(
      v + *slack >= v.lower_bound(), absl::StrCat(v.name(), "_lb"));
  model_.set_lower_bound(v, -kInfinity);
  return {*slack, lower_bound};
}

absl::StatusOr<std::unique_ptr<SDCScheduler>> SDCScheduler::Create(
    FunctionBase* f, const DelayEstimator& delay_estimator) {
  XLS_ASSIGN_OR_RETURN(DelayMap delay_map,
                       ComputeNodeDelays(f, delay_estimator));
  std::unique_ptr<SDCScheduler> scheduler(
      new SDCScheduler(f, std::move(delay_map)));
  XLS_RETURN_IF_ERROR(scheduler->Initialize());
  return std::move(scheduler);
}

SDCScheduler::SDCScheduler(FunctionBase* f, DelayMap delay_map)
    : f_(f),
      delay_map_(std::move(delay_map)),
      model_(f, delay_map_, absl::StrCat("sdc_model:", f->name())) {}

absl::Status SDCScheduler::Initialize() {
  XLS_ASSIGN_OR_RETURN(
      solver_, math_opt::IncrementalSolver::New(&model_.UnderlyingModel(),
                                                math_opt::SolverType::kGlop));

  for (Node* node : f_->nodes()) {
    for (Node* user : node->users()) {
      XLS_RETURN_IF_ERROR(model_.AddDefUseConstraints(node, user));
    }
    if (f_->IsFunction() && f_->HasImplicitUse(node)) {
      XLS_RETURN_IF_ERROR(model_.AddDefUseConstraints(node, std::nullopt));
    }
  }

  if (f_->IsProc()) {
    Proc* proc = f_->AsProcOrDie();
    for (int64_t index = 0; index < proc->GetStateElementCount(); ++index) {
      Param* const state_access = proc->GetStateParam(index);
      Node* const next_state_element = proc->GetNextStateElement(index);

      // The next-state element always has lifetime extended to the state param
      // node, since we can't store the new value in the state register until
      // the old value's been used.
      XLS_RETURN_IF_ERROR(
          model_.AddLifetimeConstraint(next_state_element, state_access));
    }
  }
  return absl::OkStatus();
}

absl::Status SDCScheduler::AddConstraints(
    absl::Span<const SchedulingConstraint> constraints) {
  for (const SchedulingConstraint& constraint : constraints) {
    XLS_RETURN_IF_ERROR(model_.AddSchedulingConstraint(constraint));
  }
  return absl::OkStatus();
}

absl::Status SDCScheduler::BuildError(
    const math_opt::SolveResult& result,
    SchedulingFailureBehavior failure_behavior) {
  XLS_CHECK_NE(result.termination.reason,
               math_opt::TerminationReason::kOptimal);

  if (failure_behavior.explain_infeasibility &&
      (result.termination.reason == math_opt::TerminationReason::kInfeasible ||
       result.termination.reason ==
           math_opt::TerminationReason::kInfeasibleOrUnbounded)) {
    XLS_RETURN_IF_ERROR(model_.AddSlackVariables(
        failure_behavior.infeasible_per_state_backedge_slack_pool));
    XLS_ASSIGN_OR_RETURN(math_opt::SolveResult result_with_slack,
                         solver_->Solve());
    if (result_with_slack.termination.reason ==
            math_opt::TerminationReason::kOptimal ||
        result_with_slack.termination.reason ==
            math_opt::TerminationReason::kFeasible) {
      XLS_RETURN_IF_ERROR(
          model_.ExtractError(result_with_slack.variable_values()));
    }
  }

  // We don't know why the solver failed to find an optimal solution to our LP
  // problem; it could be an infeasibility issue (which needs more work to
  // analyze), a timeout, a precision error, or more. For now, just return a
  // simple error hinting at the problem.
  return absl::InternalError(
      absl::StrCat("The problem does not have an optimal solution; solver "
                   "terminated with ",
                   math_opt::EnumToString(result.termination.reason)));
}

absl::StatusOr<ScheduleCycleMap> SDCScheduler::Schedule(
    std::optional<int64_t> pipeline_stages, int64_t clock_period_ps,
    SchedulingFailureBehavior failure_behavior, bool check_feasibility) {
  model_.SetClockPeriod(clock_period_ps);

  model_.SetPipelineLength(pipeline_stages);
  if (!pipeline_stages.has_value() && !check_feasibility) {
    // Find the minimum feasible pipeline length.
    model_.MinimizePipelineLength();
    XLS_ASSIGN_OR_RETURN(
        const math_opt::SolveResult result_with_minimized_pipeline_length,
        solver_->Solve());
    if (result_with_minimized_pipeline_length.termination.reason !=
        math_opt::TerminationReason::kOptimal) {
      return BuildError(result_with_minimized_pipeline_length,
                        failure_behavior);
    }
    XLS_ASSIGN_OR_RETURN(
        const int64_t min_pipeline_length,
        model_.ExtractPipelineLength(
            result_with_minimized_pipeline_length.variable_values()));
    model_.SetPipelineLength(min_pipeline_length);
  }

  if (check_feasibility) {
    model_.RemoveObjective();
  } else {
    model_.SetObjective();
  }

  XLS_ASSIGN_OR_RETURN(math_opt::SolveResult result, solver_->Solve());
  if (result.termination.reason == math_opt::TerminationReason::kOptimal ||
      (check_feasibility &&
       result.termination.reason == math_opt::TerminationReason::kFeasible)) {
    return model_.ExtractResult(result.variable_values());
  }
  return BuildError(result, failure_behavior);
}

}  // namespace xls

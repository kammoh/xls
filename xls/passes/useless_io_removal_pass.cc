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

#include "xls/passes/useless_io_removal_pass.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/node_iterator.h"
#include "xls/ir/node_util.h"
#include "xls/ir/nodes.h"
#include "xls/ir/op.h"
#include "xls/ir/package.h"
#include "xls/ir/value_helpers.h"
#include "xls/passes/optimization_pass.h"

namespace xls {

namespace {

using ChannelToSendMap =
    absl::flat_hash_map<std::string, absl::flat_hash_set<Send*>>;
using ChannelToReceiveMap =
    absl::flat_hash_map<std::string, absl::flat_hash_set<Receive*>>;

struct ChannelMaps {
  ChannelToSendMap to_send;
  ChannelToReceiveMap to_receive;
};

absl::StatusOr<ChannelMaps> ComputeChannelMaps(Package* package) {
  ChannelMaps result;
  if (package->channels().empty()) {
    return result;
  }
  for (std::unique_ptr<Proc>& proc : package->procs()) {
    for (Node* node : proc->nodes()) {
      if (node->Is<Send>()) {
        result.to_send[node->As<Send>()->channel_name()].insert(
            node->As<Send>());
      }
      if (node->Is<Receive>()) {
        result.to_receive[node->As<Receive>()->channel_name()].insert(
            node->As<Receive>());
      }
    }
  }
  return result;
}

}  // namespace

absl::StatusOr<bool> UselessIORemovalPass::RunInternal(
    Package* p, const OptimizationPassOptions& options,
    PassResults* results) const {
  bool changed = false;
  XLS_ASSIGN_OR_RETURN(ChannelMaps channel_maps, ComputeChannelMaps(p));
  // Remove send_if and recv_if with literal false conditions, unless they are
  // the last send/receive on that channel.
  // Replace send_if and recv_if with literal true conditions with unpredicated
  // send and recv.
  for (std::unique_ptr<Proc>& proc : p->procs()) {
    for (Node* node : TopoSort(proc.get())) {
      Node* replacement = nullptr;
      if (node->Is<Send>()) {
        Send* send = node->As<Send>();
        if (!send->predicate().has_value()) {
          continue;
        }
        Node* predicate = send->predicate().value();
        if (IsLiteralZero(predicate) &&
            channel_maps.to_send.at(send->channel_name()).size() >= 2) {
          channel_maps.to_send.at(send->channel_name()).erase(send);
          replacement = send->token();
        } else if (IsLiteralUnsignedOne(predicate)) {
          XLS_ASSIGN_OR_RETURN(
              replacement,
              proc->MakeNode<Send>(node->loc(), send->token(), send->data(),
                                   /*predicate=*/std::nullopt,
                                   send->channel_name()));
        }
      } else if (node->Is<Receive>()) {
        Receive* receive = node->As<Receive>();
        if (!receive->predicate().has_value()) {
          continue;
        }
        Node* predicate = receive->predicate().value();
        if (IsLiteralZero(predicate) &&
            channel_maps.to_receive.at(receive->channel_name()).size() >= 2) {
          XLS_ASSIGN_OR_RETURN(Channel * channel, GetChannelUsedByNode(node));
          channel_maps.to_receive.at(receive->channel_name()).erase(receive);
          XLS_ASSIGN_OR_RETURN(Literal * zero,
                               proc->MakeNode<Literal>(
                                   node->loc(), ZeroOfType(channel->type())));
          XLS_ASSIGN_OR_RETURN(
              replacement,
              proc->MakeNode<Tuple>(
                  node->loc(), std::vector<Node*>{receive->token(), zero}));
        } else if (IsLiteralUnsignedOne(predicate)) {
          XLS_ASSIGN_OR_RETURN(replacement, proc->MakeNode<Receive>(
                                                node->loc(), receive->token(),
                                                /*predicate=*/std::nullopt,
                                                receive->channel_name(),
                                                receive->is_blocking()));
        }
      }
      if (replacement != nullptr) {
        XLS_RETURN_IF_ERROR(node->ReplaceUsesWith(replacement));
        XLS_RETURN_IF_ERROR(proc->RemoveNode(node));
        changed = true;
      }
    }
  }
  return changed;
}

}  // namespace xls

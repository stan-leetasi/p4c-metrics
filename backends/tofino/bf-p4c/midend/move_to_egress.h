/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_MOVE_TO_EGRESS_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_MOVE_TO_EGRESS_H_

#include "backends/tofino/bf-p4c/midend/defuse.h"
#include "backends/tofino/bf-p4c/midend/type_checker.h"
#include "ir/ir.h"

class MoveToEgress : public PassManager {
    BFN::EvaluatorPass *evaluator;
    ordered_set<const IR::P4Parser *> ingress_parser, egress_parser;
    ordered_set<const IR::P4Control *> ingress, egress, ingress_deparser, egress_deparser;
    ComputeDefUse defuse;

    class FindIngressPacketMods;

 public:
    explicit MoveToEgress(BFN::EvaluatorPass *ev);
};

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_MOVE_TO_EGRESS_H_ */

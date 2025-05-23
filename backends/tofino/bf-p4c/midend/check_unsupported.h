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

#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_UNSUPPORTED_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_UNSUPPORTED_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {
class TypeMap;
}  // namespace P4

namespace BFN {

/**
 * \ingroup midend
 * \brief Check for unsupported features in the backend compiler.
 */
class CheckUnsupported final : public Inspector {
    bool preorder(const IR::PathExpression *path_expression) override;
    void postorder(const IR::P4Table *) override;
    bool preorder(const IR::Declaration_Instance *instance) override;

 public:
    explicit CheckUnsupported(P4::ReferenceMap *, P4::TypeMap *) {}
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_CHECK_UNSUPPORTED_H_ */

/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "midend.h"

#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "lower.h"
#include "midend/actionSynthesis.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateInvalidHeaders.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateTuples.h"
#include "midend/expandEmit.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/noMatch.h"
#include "midend/parserUnroll.h"
#include "midend/removeExits.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/singleArgumentSelect.h"
#include "midend/tableHit.h"
#include "midend/validateProperties.h"

namespace P4::EBPF {

using namespace P4::literals;

class EnumOn32Bits : public P4::ChooseEnumRepresentation {
    bool convert(const IR::Type_Enum *type) const override {
        if (type->srcInfo.isValid()) {
            auto sourceFile = type->srcInfo.getSourceFile();
            if (sourceFile.endsWith("_model.p4"))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override { return 32; }
};

const IR::ToplevelBlock *MidEnd::run(EbpfOptions &options, const IR::P4Program *program,
                                     std::ostream *outStream) {
    if (program == nullptr && options.listMidendPasses == 0) return nullptr;

    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);

    PassManager midEnd = {};
    if (options.loadIRFromJson == false) {
        midEnd.addPasses(
            {new P4::ConvertEnums(&typeMap, new EnumOn32Bits()),
             new P4::ClearTypeMap(&typeMap),
             new P4::RemoveMiss(&typeMap),
             new P4::EliminateInvalidHeaders(&typeMap),
             new P4::EliminateNewtype(&typeMap),
             new P4::SimplifyControlFlow(&typeMap, true),
             new P4::SimplifyKey(
                 &typeMap, new P4::OrPolicy(new P4::IsValid(&typeMap), new P4::IsLikeLeftValue())),
             new P4::RemoveExits(&typeMap),
             new P4::ConstantFolding(&typeMap),
             new P4::SimplifySelectCases(&typeMap, false),  // accept non-constant keysets
             new P4::ExpandEmit(&typeMap),
             new P4::HandleNoMatch(),
             new P4::SimplifyParsers(),
             new PassRepeated({
                 new P4::ConstantFolding(&typeMap),
                 new P4::StrengthReduction(&typeMap),
             }),
             new P4::SimplifyComparisons(&typeMap),
             new P4::EliminateTuples(&typeMap),
             new P4::SimplifySelectList(&typeMap),
             new P4::MoveDeclarations(),  // more may have been introduced
             new P4::RemoveSelectBooleans(&typeMap),
             new P4::SingleArgumentSelect(&typeMap),
             new P4::ConstantFolding(&typeMap),
             new P4::SimplifyControlFlow(&typeMap, true),
             new P4::TableHit(&typeMap),
             new P4::RemoveLeftSlices(&typeMap),
             new EBPF::Lower(&refMap, &typeMap),
             new P4::ParsersUnroll(true, &refMap, &typeMap),
             evaluator,
             new P4::MidEndLast()});

        if (options.arch == "psa") {
            midEnd.addPasses({new P4::ValidateTableProperties(
                {"size"_cs, "psa_direct_counter"_cs, "psa_direct_meter"_cs,
                 "psa_empty_group_action"_cs, "psa_implementation"_cs})});
        } else {
            midEnd.addPasses({new P4::ValidateTableProperties({"size"_cs, "implementation"_cs})});
        }

        if (options.listMidendPasses) {
            midEnd.listPasses(*outStream, cstring::newline);
            *outStream << std::endl;
        }
        if (options.excludeMidendPasses) {
            midEnd.removePasses(options.passesToExcludeMidend);
        }
    } else {
        midEnd.addPasses({new P4::ResolveReferences(&refMap),
                          new P4::TypeChecking(&refMap, &typeMap), evaluator});
    }
    midEnd.setName("MidEnd");
    midEnd.addDebugHooks(hooks);
    program = program->apply(midEnd);
    if (::P4::errorCount() > 0) return nullptr;

    return evaluator->getToplevelBlock();
}

}  // namespace P4::EBPF

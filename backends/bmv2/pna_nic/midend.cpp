/*
Copyright 2024 Marvell Technology, Inc.

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

#include "backends/bmv2/common/check_unsupported.h"
#include "backends/bmv2/pna_nic/options.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4-14/fromv1.0/v1model.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/simplifyParsers.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/uniqueNames.h"
#include "frontends/p4/unusedDeclarations.h"
#include "midend/actionSynthesis.h"
#include "midend/compileTimeOps.h"
#include "midend/complexComparison.h"
#include "midend/convertEnums.h"
#include "midend/copyStructures.h"
#include "midend/eliminateInvalidHeaders.h"
#include "midend/eliminateNewtype.h"
#include "midend/eliminateSerEnums.h"
#include "midend/eliminateSwitch.h"
#include "midend/eliminateTuples.h"
#include "midend/expandEmit.h"
#include "midend/expandLookahead.h"
#include "midend/fillEnumMap.h"
#include "midend/flattenHeaders.h"
#include "midend/flattenInterfaceStructs.h"
#include "midend/local_copyprop.h"
#include "midend/midEndLast.h"
#include "midend/nestedStructs.h"
#include "midend/orderArguments.h"
#include "midend/predication.h"
#include "midend/removeAssertAssume.h"
#include "midend/removeLeftSlices.h"
#include "midend/removeMiss.h"
#include "midend/removeSelectBooleans.h"
#include "midend/removeUnusedParameters.h"
#include "midend/replaceSelectRange.h"
#include "midend/simplifyExternMethod.h"
#include "midend/simplifyKey.h"
#include "midend/simplifySelectCases.h"
#include "midend/simplifySelectList.h"
#include "midend/tableHit.h"
#include "midend/validateProperties.h"

namespace P4::BMV2 {

using namespace P4::literals;

/// This class implements a policy suitable for the ConvertEnums pass.
/// The policy is: convert all enums that are not part of the psa.
/// Use 32-bit values for all enums.
/// Also convert PNA_PacketPath_t to bit<32>
class PnaEnumOn32Bits : public P4::ChooseEnumRepresentation {
    cstring filename;

    bool convert(const IR::Type_Enum *type) const override {
        if (type->name == "PNA_PacketPath_t") return true;
        if (type->name == "PNA_MeterColor_t") return true;
        if (type->name == "PNA_Direction_t") return true;
        if (type->srcInfo.isValid()) {
            auto sourceFile = type->srcInfo.getSourceFile();
            if (sourceFile.endsWith(filename))
                // Don't convert any of the standard enums
                return false;
        }
        return true;
    }
    unsigned enumSize(unsigned) const override { return 32; }

 public:
    explicit PnaEnumOn32Bits(cstring filename) : filename(filename) {}
};

PnaNicMidEnd::PnaNicMidEnd(CompilerOptions &options, std::ostream *outStream)
    : PortableMidEnd(options) {
    auto convertEnums = new P4::ConvertEnums(&typeMap, new PnaEnumOn32Bits("pna.p4"_cs));
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    P4::LocalCopyPropPolicyCallbackFn policy = [=](const Context *, const IR::Expression *e,
                                                   const DeclarationLookup *refMap) -> bool {
        auto mce = e->to<IR::MethodCallExpression>();
        if (mce == nullptr) return true;
        auto mi = P4::MethodInstance::resolve(mce, refMap, &typeMap);
        auto em = mi->to<P4::ExternMethod>();
        if (em == nullptr) return true;
        if (em->originalExternType->name.name == "Register" || em->method->name.name == "read")
            return false;
        if (em->originalExternType->name.name == "Meter" && em->method->name.name == "execute")
            return false;
        return true;
    };
    if (BMV2::PnaNicContext::get().options().loadIRFromJson == false) {
        addPasses({
            options.ndebug ? new P4::RemoveAssertAssume(&typeMap) : nullptr,
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::SimplifyExternMethodCalls(&typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new CheckUnsupported(),
            new P4::RemoveMiss(&typeMap),
            new P4::EliminateNewtype(&typeMap),
            new P4::EliminateInvalidHeaders(&typeMap),
            new P4::EliminateSerEnums(&typeMap),
            convertEnums,
            [this, convertEnums]() { enumMap = convertEnums->getEnumMapping(); },
            new P4::OrderArguments(&typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::SimplifyKey(&typeMap,
                                new P4::OrPolicy(new P4::IsValid(&typeMap), new P4::IsMask())),
            new P4::ConstantFolding(&typeMap),
            new P4::StrengthReduction(&typeMap),
            new P4::SimplifySelectCases(&typeMap, true),  // require constant keysets
            new P4::ExpandLookahead(&typeMap),
            new P4::ExpandEmit(&typeMap),
            new P4::SimplifyParsers(),
            new P4::StrengthReduction(&typeMap),
            new P4::EliminateTuples(&typeMap),
            new P4::SimplifyComparisons(&typeMap),
            new P4::CopyStructures(&typeMap),
            new P4::NestedStructs(&typeMap),
            new P4::SimplifySelectList(&typeMap),
            new P4::RemoveSelectBooleans(&typeMap),
            new P4::FlattenHeaders(&typeMap),
            new P4::FlattenInterfaceStructs(&typeMap),
            new P4::ReplaceSelectRange(),
            new P4::Predication(),
            new P4::MoveDeclarations(),  // more may have been introduced
            new P4::ConstantFolding(&typeMap),
            new P4::LocalCopyPropagation(&typeMap, nullptr, policy),
            new PassRepeated(
                {new P4::ConstantFolding(&typeMap), new P4::StrengthReduction(&typeMap)}),
            new P4::MoveDeclarations(),
            new P4::ValidateTableProperties({"pna_implementation"_cs, "pna_direct_counter"_cs,
                                             "pna_direct_meter"_cs, "pna_idle_timeout"_cs,
                                             "size"_cs}),
            new P4::SimplifyControlFlow(&typeMap, true),
            new P4::CompileTimeOperations(),
            new P4::TableHit(&typeMap),
            new P4::EliminateSwitch(&typeMap),
            new P4::MoveActionsToTables(&refMap, &typeMap),
            new P4::RemoveLeftSlices(&typeMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new P4::MidEndLast(),
            evaluator,
            [this, evaluator]() { toplevel = evaluator->getToplevelBlock(); },
        });
        if (options.listMidendPasses) {
            listPasses(*outStream, cstring::newline);
            *outStream << std::endl;
            return;
        }
        if (options.excludeMidendPasses) {
            removePasses(options.passesToExcludeMidend);
        }
    } else {
        auto fillEnumMap = new P4::FillEnumMap(new PnaEnumOn32Bits("pna.p4"_cs), &typeMap);
        addPasses({
            new P4::ResolveReferences(&refMap),
            new P4::TypeChecking(&refMap, &typeMap),
            new CheckUnsupported(),
            fillEnumMap,
            [this, fillEnumMap]() { enumMap = fillEnumMap->repr; },
            evaluator,
            [this, evaluator]() { toplevel = evaluator->getToplevelBlock(); },
        });
    }
}

}  // namespace P4::BMV2

#include "backends/p4tools/modules/testgen/targets/pna/target.h"

#include <cstddef>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/compiler_result.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/pna/dpdk/program_info.h"
#include "backends/p4tools/modules/testgen/targets/pna/test_backend.h"

namespace P4::P4Tools::P4Testgen::Pna {

/* =============================================================================================
 *  PnaDpdkTestgenTarget implementation
 * ============================================================================================= */

PnaDpdkTestgenTarget::PnaDpdkTestgenTarget() : TestgenTarget("dpdk", "pna") {}

void PnaDpdkTestgenTarget::make() {
    static PnaDpdkTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new PnaDpdkTestgenTarget();
    }
}

const PnaDpdkProgramInfo *PnaDpdkTestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    const auto *mainType = mainDecl->type->to<IR::Type_Specialized>();
    if (mainType == nullptr || mainType->baseType->path->name != "PNA_NIC") {
        error(ErrorType::ERR_INVALID,
              "%1%: This P4Testgen back end only supports a 'PNA_NIC' main package. The current "
              "type is %2%",
              mainDecl, mainDecl->type);
        return nullptr;
    }
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have four arguments.
    if (blocks.size() != 4) {
        error(ErrorType::ERR_INVALID, "%1%: The PNA architecture requires 4 pipes. Received %2%.",
              mainDecl, blocks.size());
        return nullptr;
    }

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = PnaDpdkProgramInfo::ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    return new PnaDpdkProgramInfo(*compilerResult.checkedTo<TestgenCompilerResult>(),
                                  programmableBlocks);
}

PnaTestBackend *PnaDpdkTestgenTarget::getTestBackendImpl(
    const ProgramInfo &programInfo, const TestBackendConfiguration &testBackendConfiguration,
    SymbolicExecutor &symbex) const {
    return new PnaTestBackend(programInfo, testBackendConfiguration, symbex);
}

PnaDpdkCmdStepper *PnaDpdkTestgenTarget::getCmdStepperImpl(ExecutionState &state,
                                                           AbstractSolver &solver,
                                                           const ProgramInfo &programInfo) const {
    return new PnaDpdkCmdStepper(state, solver, programInfo);
}

PnaDpdkExprStepper *PnaDpdkTestgenTarget::getExprStepperImpl(ExecutionState &state,
                                                             AbstractSolver &solver,
                                                             const ProgramInfo &programInfo) const {
    return new PnaDpdkExprStepper(state, solver, programInfo);
}

MidEnd PnaDpdkTestgenTarget::mkMidEnd(const CompilerOptions &options) const {
    MidEnd midEnd(options);
    midEnd.addDefaultPasses();

    return midEnd;
}

}  // namespace P4::P4Tools::P4Testgen::Pna

#include "Checks.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/CommandLine.h"

using namespace clang::tooling;
using namespace clang::ast_matchers;

static llvm::cl::OptionCategory ToolCategory("static-analyzer options");

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());

    MatchFinder Finder;

    registerUnusedReturnValueCheck(Finder);
    registerUnusedVariableCheck(Finder);
    registerUnusedParameterCheck(Finder);
    registerUnusedTypeCheck(Finder);
    registerUnusedFunctionCheck(Finder);
    registerMissingElseCheck(Finder);
    registerThrowPointerCheck(Finder);
    registerHiddenFunctionCheck(Finder);
    registerDanglingAssignmentCheck(Finder);
    registerDynamicTypeInCtorDtorCheck(Finder);
    registerExplicitBaseInitCheck(Finder);
    registerExplicitConstructorCheck(Finder);
    registerUninitializedMemberCheck(Finder);
    registerSelfAssignmentCheck(Finder);
    registerVirtualSpecifierCheck(Finder);
    registerDefaultArgOverrideCheck(Finder);
    registerFunctionTemplateSpecializationCheck(Finder);
    registerNoexceptCheck(Finder);
    registerCStyleCastCheck(Finder);
    registerConstCastCheck(Finder);
    registerReinterpretCastCheck(Finder);
    registerDynamicMemoryCheck(Finder);
    registerMovedFromStateCheck(Finder);
    registerVirtualBaseCastCheck(Finder);
    registerIntToPointerCastCheck(Finder);
    registerOverlappingCopyCheck(Finder);
    registerAdvancedMemoryManagementCheck(Finder);
    registerMixedVirtualInheritanceCheck(Finder);
    registerVirtualMemberPtrComparisonCheck(Finder);
    registerDependentBaseLookupCheck(Finder);
    registerRefQualifiedMemberCheck(Finder);

    return Tool.run(newFrontendActionFactory(&Finder).get());
}

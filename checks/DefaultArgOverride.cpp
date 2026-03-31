#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class DefaultArgOverrideCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *MD = Result.Nodes.getNodeAs<CXXMethodDecl>("overrideMethod");
        if (!MD) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(MD->getBeginLoc())) return;
        if (!MD->getDeclName().isIdentifier()) return;
        for (const auto *BaseMD : MD->overridden_methods()) {
            unsigned NumParams = MD->getNumParams();
            if (BaseMD->getNumParams() != NumParams) continue;
            for (unsigned i = 0; i < NumParams; ++i) {
                bool derivedHasDefault = MD->getParamDecl(i)->hasDefaultArg();
                bool baseHasDefault    = BaseMD->getParamDecl(i)->hasDefaultArg();
                if (derivedHasDefault != baseHasDefault) {
                    auto Loc = SM.getPresumedLoc(MD->getBeginLoc());
                    if (!Loc.isValid()) return;
                    llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                                 << ": [HSCAN.3.2] '" << MD->getName()
                                 << "' has a different default argument than the base."
                                 << " Default args are resolved by static type, so"
                                 << " callers get different values depending on which"
                                 << " reference they use.\n";
                    return;
                }
            }
        }
    }
};
static DefaultArgOverrideCallback Callback;
} // namespace

void registerDefaultArgOverrideCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxMethodDecl(isOverride(), isDefinition()).bind("overrideMethod"),
        &Callback
    );
}

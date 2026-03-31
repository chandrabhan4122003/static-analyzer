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
                const auto *DerivedParam = MD->getParamDecl(i);
                const auto *BaseParam   = BaseMD->getParamDecl(i);

                bool derivedHasDefault = DerivedParam->hasDefaultArg();
                bool baseHasDefault    = BaseParam->hasDefaultArg();

                if (derivedHasDefault != baseHasDefault) {
                    auto Loc = SM.getPresumedLoc(MD->getBeginLoc());
                    if (!Loc.isValid()) return;
                    llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                                 << ": [HSCAN.3.2] Overriding function '"
                                 << MD->getName()
                                 << "' has different default argument than base\n";
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

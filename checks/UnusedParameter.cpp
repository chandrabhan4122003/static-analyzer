#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class UnusedParameterCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Param = Result.Nodes.getNodeAs<ParmVarDecl>("unusedParam");
        if (!Param) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Param->getBeginLoc())) return;
        if (!Param->getIdentifier()) return;
        if (Param->hasAttr<UnusedAttr>()) return;
        if (Param->isReferenced()) return;
        auto Loc = SM.getPresumedLoc(Param->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAA.2.2] Parameter '" << Param->getName()
                     << "' is named but never used in the function body."
                     << " Either use it, remove the name, or mark [[maybe_unused]].\n";
    }
};
static UnusedParameterCallback Callback;
} // namespace

void registerUnusedParameterCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        parmVarDecl(unless(hasAttr(attr::Unused))).bind("unusedParam"),
        &Callback
    );
}

#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class CStyleCastCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;

        if (const auto *Cast = Result.Nodes.getNodeAs<CStyleCastExpr>("ccast")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            // Exception: (void) cast is allowed
            if (Cast->getType()->isVoidType()) return;
            auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.2.2] C-style cast shall not be used\n";
        }
    }
};

static CStyleCastCallback Callback;

} // namespace

void registerCStyleCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cStyleCastExpr().bind("ccast"),
        &Callback
    );
}

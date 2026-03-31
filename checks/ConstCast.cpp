#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class ConstCastCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Cast = Result.Nodes.getNodeAs<CXXConstCastExpr>("constCast");
        if (!Cast) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Cast->getBeginLoc())) return;

        auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAI.2.3] const_cast removes const/volatile qualification\n";
    }
};

static ConstCastCallback Callback;

} // namespace

void registerConstCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxConstCastExpr().bind("constCast"),
        &Callback
    );
}

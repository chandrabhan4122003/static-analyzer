#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class MovedFromStateCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Use = Result.Nodes.getNodeAs<DeclRefExpr>("movedFromUse");
        if (!Use) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Use->getBeginLoc())) return;
        if (!Use->getDecl()->getDeclName().isIdentifier()) return;
        auto Loc = SM.getPresumedLoc(Use->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCBC.6.3] '" << Use->getDecl()->getName()
                     << "' is used after std::move(). Its value is now unknown."
                     << " Don't use a moved-from object.\n";
    }
};
static MovedFromStateCallback Callback;
} // namespace

void registerMovedFromStateCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        declRefExpr(
            to(varDecl(hasDescendant(
                callExpr(callee(functionDecl(hasName("::std::move"))))
            ))),
            hasAncestor(compoundStmt())
        ).bind("movedFromUse"),
        &Callback
    );
}

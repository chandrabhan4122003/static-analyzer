#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class MissingElseCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *If = Result.Nodes.getNodeAs<IfStmt>("missingElse");
        if (!If) return;

        auto &SM = *Result.SourceManager;
        if (SM.isInSystemHeader(If->getBeginLoc())) return;

        auto Loc = SM.getPresumedLoc(If->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAJ.4.1] if...else if construct missing final else\n";
    }
};

static MissingElseCallback Callback;

} // namespace

void registerMissingElseCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        ifStmt(
            hasElse(ifStmt(unless(hasElse(stmt()))))
        ).bind("missingElse"),
        &Callback
    );
}

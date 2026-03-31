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
        const auto *Cast = Result.Nodes.getNodeAs<CStyleCastExpr>("ccast");
        if (!Cast) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Cast->getBeginLoc())) return;
        if (Cast->getType()->isVoidType()) return;
        auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAI.2.2] C-style cast used. C-style casts are unsafe"
                     << " because they silently perform any conversion."
                     << " Use static_cast, dynamic_cast, const_cast,"
                     << " or reinterpret_cast to make intent explicit.\n";
    }
};
static CStyleCastCallback Callback;
} // namespace

void registerCStyleCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(cStyleCastExpr().bind("ccast"), &Callback);
}

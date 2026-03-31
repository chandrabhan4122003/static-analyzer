#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class AdvancedMemoryCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;
        if (const auto *NE = Result.Nodes.getNodeAs<CXXNewExpr>("placementNew")) {
            if (!SM.isInMainFile(NE->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(NE->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.3] Placement new is advanced memory management."
                         << " It requires careful handling of object lifetime and"
                         << " alignment. Avoid unless absolutely necessary.\n";
            return;
        }
        if (const auto *Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("dtorCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.3] Explicit destructor call is dangerous."
                         << " The object will be destroyed again at end of scope."
                         << " Only use with placement new.\n";
        }
    }
};
static AdvancedMemoryCallback Callback;
} // namespace

void registerAdvancedMemoryManagementCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxNewExpr(hasAnyPlacementArg(expr())).bind("placementNew"), &Callback);
    Finder.addMatcher(
        cxxMemberCallExpr(callee(cxxDestructorDecl())).bind("dtorCall"), &Callback);
}

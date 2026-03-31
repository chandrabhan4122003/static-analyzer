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

        // Placement new
        if (const auto *NE = Result.Nodes.getNodeAs<CXXNewExpr>("placementNew")) {
            if (!SM.isInMainFile(NE->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(NE->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.3] Placement new is advanced memory management\n";
            return;
        }

        // Explicit destructor call
        if (const auto *Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("dtorCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.3] Explicit destructor call is advanced"
                         << " memory management\n";
        }
    }
};

static AdvancedMemoryCallback Callback;

} // namespace

void registerAdvancedMemoryManagementCheck(MatchFinder &Finder) {
    // Placement new — has placement args
    Finder.addMatcher(
        cxxNewExpr(hasAnyPlacementArg(expr())).bind("placementNew"),
        &Callback
    );
    // Explicit destructor call
    Finder.addMatcher(
        cxxMemberCallExpr(
            callee(cxxDestructorDecl())
        ).bind("dtorCall"),
        &Callback
    );
}

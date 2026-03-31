#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class DynamicTypeInCtorDtorCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;

        // Virtual call in ctor/dtor
        if (const auto *Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("virtualCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            const auto *MD = Call->getMethodDecl();
            if (!MD || !MD->isVirtual()) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.1.1] Virtual function called from constructor/destructor\n";
            return;
        }

        // dynamic_cast in ctor/dtor
        if (const auto *Cast = Result.Nodes.getNodeAs<CXXDynamicCastExpr>("dynCast")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.1.1] dynamic_cast used in constructor/destructor\n";
        }
    }
};

static DynamicTypeInCtorDtorCallback Callback;

} // namespace

void registerDynamicTypeInCtorDtorCheck(MatchFinder &Finder) {
    // Virtual call inside ctor
    Finder.addMatcher(
        cxxMemberCallExpr(
            callee(cxxMethodDecl(isVirtual())),
            hasAncestor(cxxConstructorDecl())
        ).bind("virtualCall"),
        &Callback
    );
    // Virtual call inside dtor
    Finder.addMatcher(
        cxxMemberCallExpr(
            callee(cxxMethodDecl(isVirtual())),
            hasAncestor(cxxDestructorDecl())
        ).bind("virtualCall"),
        &Callback
    );
    // dynamic_cast inside ctor
    Finder.addMatcher(
        cxxDynamicCastExpr(
            hasAncestor(cxxConstructorDecl())
        ).bind("dynCast"),
        &Callback
    );
    // dynamic_cast inside dtor
    Finder.addMatcher(
        cxxDynamicCastExpr(
            hasAncestor(cxxDestructorDecl())
        ).bind("dynCast"),
        &Callback
    );
}

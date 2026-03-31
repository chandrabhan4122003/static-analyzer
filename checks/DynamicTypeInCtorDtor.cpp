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
        if (const auto *Call = Result.Nodes.getNodeAs<CXXMemberCallExpr>("virtualCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            const auto *MD = Call->getMethodDecl();
            if (!MD || !MD->isVirtual()) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.1.1] Virtual function called inside constructor"
                         << " or destructor. During construction/destruction the"
                         << " object's dynamic type is not fully resolved, so the"
                         << " base class version will be called, not the overridden one."
                         << " Use two-phase initialisation instead.\n";
            return;
        }
        if (const auto *Cast = Result.Nodes.getNodeAs<CXXDynamicCastExpr>("dynCast")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.1.1] dynamic_cast used inside constructor or"
                         << " destructor. The object is not fully constructed yet,"
                         << " so dynamic_cast may not behave as expected.\n";
        }
    }
};
static DynamicTypeInCtorDtorCallback Callback;
} // namespace

void registerDynamicTypeInCtorDtorCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxMemberCallExpr(callee(cxxMethodDecl(isVirtual())),
            hasAncestor(cxxConstructorDecl())).bind("virtualCall"), &Callback);
    Finder.addMatcher(
        cxxMemberCallExpr(callee(cxxMethodDecl(isVirtual())),
            hasAncestor(cxxDestructorDecl())).bind("virtualCall"), &Callback);
    Finder.addMatcher(
        cxxDynamicCastExpr(hasAncestor(cxxConstructorDecl())).bind("dynCast"), &Callback);
    Finder.addMatcher(
        cxxDynamicCastExpr(hasAncestor(cxxDestructorDecl())).bind("dynCast"), &Callback);
}

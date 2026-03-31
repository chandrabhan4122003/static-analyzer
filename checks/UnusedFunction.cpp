#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class UnusedFunctionCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("unusedFunc");
        if (!FD) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(FD->getBeginLoc())) return;
        if (const auto *MD = dyn_cast<CXXMethodDecl>(FD)) {
            if (MD->isVirtual()) return;
            if (isa<CXXConstructorDecl>(MD) || isa<CXXDestructorDecl>(MD)) return;
            if (MD->isCopyAssignmentOperator() || MD->isMoveAssignmentOperator()) return;
        }
        if (FD->hasAttr<UnusedAttr>()) return;
        if (FD->isReferenced()) return;
        if (!FD->getDeclName().isIdentifier()) return;
        auto Loc = SM.getPresumedLoc(FD->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAA.2.4] Function '" << FD->getName()
                     << "' has limited visibility (static/private/anonymous namespace)"
                     << " but is never called. This is dead code — remove it or"
                     << " check if it was accidentally left after refactoring.\n";
    }
};
static UnusedFunctionCallback Callback;
} // namespace

void registerUnusedFunctionCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        functionDecl(
            anyOf(isStaticStorageClass(), hasParent(decl())),
            isDefinition()
        ).bind("unusedFunc"),
        &Callback
    );
}

#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTContext.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class DanglingAssignmentCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Assign = Result.Nodes.getNodeAs<BinaryOperator>("danglingAssign");
        if (!Assign) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Assign->getBeginLoc())) return;

        const auto *LHS = Assign->getLHS()->IgnoreParenImpCasts();
        const auto *RHS = Assign->getRHS()->IgnoreParenImpCasts();

        // RHS must be address-of a local variable
        const UnaryOperator *AddrOf = dyn_cast<UnaryOperator>(RHS);
        if (!AddrOf || AddrOf->getOpcode() != UO_AddrOf) return;

        const auto *Local = dyn_cast<DeclRefExpr>(AddrOf->getSubExpr()->IgnoreParenImpCasts());
        if (!Local) return;

        const auto *LocalVar = dyn_cast<VarDecl>(Local->getDecl());
        if (!LocalVar || !LocalVar->hasLocalStorage()) return;

        // LHS must be a pointer with greater lifetime
        const auto *LHSRef = dyn_cast<DeclRefExpr>(LHS);
        if (!LHSRef) return;

        const auto *LHSVar = dyn_cast<VarDecl>(LHSRef->getDecl());
        if (!LHSVar) return;

        // If LHS has static storage or is in outer scope — violation
        if (LHSVar->hasGlobalStorage() || LHSVar->isStaticLocal()) {
            auto Loc = SM.getPresumedLoc(Assign->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAG.8.3] Address of local variable assigned to"
                         << " pointer with greater lifetime\n";
        }
    }
};

static DanglingAssignmentCallback Callback;

} // namespace

void registerDanglingAssignmentCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        binaryOperator(
            hasOperatorName("="),
            hasLHS(expr(hasType(pointerType())))
        ).bind("danglingAssign"),
        &Callback
    );
}

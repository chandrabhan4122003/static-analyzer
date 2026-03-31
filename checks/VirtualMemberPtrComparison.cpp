#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class VirtualMemberPtrComparisonCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *BO = Result.Nodes.getNodeAs<BinaryOperator>("ptrCmp");
        if (!BO) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(BO->getBeginLoc())) return;

        // Check if either side is a pointer to a virtual member function
        auto checkSide = [&](const Expr *E) -> bool {
            E = E->IgnoreParenImpCasts();
            const auto *UO = dyn_cast<UnaryOperator>(E);
            if (!UO || UO->getOpcode() != UO_AddrOf) return false;
            const auto *DRE = dyn_cast<DeclRefExpr>(UO->getSubExpr());
            if (!DRE) return false;
            const auto *MD = dyn_cast<CXXMethodDecl>(DRE->getDecl());
            if (!MD) return false;
            return MD->isVirtual();
        };

        // Also check if RHS is nullptr — that's allowed
        const Expr *RHS = BO->getRHS()->IgnoreParenImpCasts();
        if (RHS->isNullPointerConstant(*Result.Context,
            Expr::NPC_ValueDependentIsNull) != Expr::NPCK_NotNull) return;

        if (checkSide(BO->getLHS()) || checkSide(BO->getRHS())) {
            auto Loc = SM.getPresumedLoc(BO->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.4] Comparison of virtual member function"
                         << " pointer with non-null value is unspecified\n";
        }
    }
};

static VirtualMemberPtrComparisonCallback Callback;

} // namespace

void registerVirtualMemberPtrComparisonCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        binaryOperator(
            anyOf(hasOperatorName("=="), hasOperatorName("!=")),
            hasLHS(expr(hasType(memberPointerType()))),
            hasRHS(expr())
        ).bind("ptrCmp"),
        &Callback
    );
}

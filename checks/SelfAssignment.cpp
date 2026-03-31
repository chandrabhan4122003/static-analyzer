#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Stmt.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class SelfAssignmentCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Op = Result.Nodes.getNodeAs<CXXMethodDecl>("assignOp");
        if (!Op || !Op->doesThisDeclarationHaveABody()) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Op->getBeginLoc())) return;

        const auto *Body = dyn_cast<CompoundStmt>(Op->getBody());
        if (!Body) return;

        // Check for self-assignment guard by looking for if(this != ...) pattern
        bool hasSelfCheck = false;
        for (const auto *S : Body->body()) {
            if (!S) continue;
            const auto *IfS = dyn_cast<IfStmt>(S);
            if (IfS) {
                hasSelfCheck = true;
                break;
            }
            // Check for swap pattern
            const auto *CallS = dyn_cast<CallExpr>(S);
            if (CallS) {
                const auto *FD = CallS->getDirectCallee();
                if (FD && FD->getDeclName().isIdentifier() &&
                    FD->getName().contains("swap")) {
                    hasSelfCheck = true;
                    break;
                }
            }
        }

        if (!hasSelfCheck) {
            auto Loc = SM.getPresumedLoc(Op->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.0.2] Assignment operator does not guard against"
                         << " self-assignment (e.g. obj = obj). Add:"
                         << " if (this != &other) { ... }\n";
        }
    }
};
static SelfAssignmentCallback Callback;
} // namespace

void registerSelfAssignmentCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxMethodDecl(
            anyOf(isCopyAssignmentOperator(), isMoveAssignmentOperator()),
            isDefinition()
        ).bind("assignOp"),
        &Callback
    );
}

#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class OverlappingCopyCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;
        if (const auto *Call = Result.Nodes.getNodeAs<CallExpr>("memcpyCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.18.1] memcpy does not handle overlapping memory."
                         << " If source and destination overlap, use memmove instead.\n";
            return;
        }
        if (const auto *Assign = Result.Nodes.getNodeAs<BinaryOperator>("unionAssign")) {
            if (!SM.isInMainFile(Assign->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Assign->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.18.1] Copying between union members may overlap."
                         << " This can cause undefined behaviour.\n";
        }
    }
};
static OverlappingCopyCallback Callback;
} // namespace

void registerOverlappingCopyCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        callExpr(callee(functionDecl(hasName("memcpy")))).bind("memcpyCall"),
        &Callback
    );
    Finder.addMatcher(
        binaryOperator(
            hasOperatorName("="),
            hasLHS(memberExpr(hasObjectExpression(
                declRefExpr(hasType(recordType(hasDeclaration(recordDecl(isUnion())))))))),
            hasRHS(memberExpr(hasObjectExpression(
                declRefExpr(hasType(recordType(hasDeclaration(recordDecl(isUnion()))))))))
        ).bind("unionAssign"),
        &Callback
    );
}

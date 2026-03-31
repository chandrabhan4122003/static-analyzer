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

        // memcpy with potentially overlapping args
        if (const auto *Call = Result.Nodes.getNodeAs<CallExpr>("memcpyCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.18.1] memcpy may copy overlapping objects;"
                         << " use memmove for overlapping regions\n";
            return;
        }

        // union member assignment — copying between union members
        if (const auto *Assign = Result.Nodes.getNodeAs<BinaryOperator>("unionAssign")) {
            if (!SM.isInMainFile(Assign->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Assign->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.18.1] Assignment between union members"
                         << " may result in overlapping copy\n";
        }
    }
};

static OverlappingCopyCallback Callback;

} // namespace

void registerOverlappingCopyCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        callExpr(callee(functionDecl(hasName("memcpy"))))
        .bind("memcpyCall"),
        &Callback
    );
    // union member to union member assignment
    Finder.addMatcher(
        binaryOperator(
            hasOperatorName("="),
            hasLHS(memberExpr(hasObjectExpression(
                declRefExpr(hasType(recordType(hasDeclaration(
                    recordDecl(isUnion())
                ))))
            ))),
            hasRHS(memberExpr(hasObjectExpression(
                declRefExpr(hasType(recordType(hasDeclaration(
                    recordDecl(isUnion())
                ))))
            )))
        ).bind("unionAssign"),
        &Callback
    );
}

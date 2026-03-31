#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class IntToPointerCastCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;
        if (const auto *Cast = Result.Nodes.getNodeAs<CXXStaticCastExpr>("intToPtr")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            QualType SrcType = Cast->getSubExpr()->getType();
            QualType DstType = Cast->getType();
            if (SrcType->isVoidPointerType() && DstType->isPointerType()) {
                auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAI.2.6] Casting void* to object pointer is unsafe."
                             << " The original type may not match the target type.\n";
                return;
            }
        }
        if (const auto *Cast = Result.Nodes.getNodeAs<CXXReinterpretCastExpr>("intToPtrReint")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            QualType SrcType = Cast->getSubExpr()->getType();
            QualType DstType = Cast->getType();
            if (SrcType->isIntegerType() && DstType->isPointerType()) {
                if (DstType->isVoidPointerType()) return;
                auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAI.2.6] Casting integer to pointer."
                             << " This is only safe for memory-mapped hardware."
                             << " Document why this is needed.\n";
            }
        }
    }
};
static IntToPointerCastCallback Callback;
} // namespace

void registerIntToPointerCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(cxxStaticCastExpr().bind("intToPtr"), &Callback);
    Finder.addMatcher(cxxReinterpretCastExpr().bind("intToPtrReint"), &Callback);
}

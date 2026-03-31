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
            // void* to pointer — violation
            if (SrcType->isVoidPointerType() && DstType->isPointerType()) {
                auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAI.2.6] Cast from void pointer to object pointer\n";
                return;
            }
        }

        if (const auto *Cast = Result.Nodes.getNodeAs<CXXReinterpretCastExpr>("intToPtrReint")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            QualType SrcType = Cast->getSubExpr()->getType();
            QualType DstType = Cast->getType();
            // integral to pointer — violation
            if (SrcType->isIntegerType() && DstType->isPointerType()) {
                // Exception: casting to void* is ok
                if (DstType->isVoidPointerType()) return;
                auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAI.2.6] Cast from integral type to pointer type\n";
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

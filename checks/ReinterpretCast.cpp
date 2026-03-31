#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class ReinterpretCastCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Cast = Result.Nodes.getNodeAs<CXXReinterpretCastExpr>("reintCast");
        if (!Cast) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Cast->getBeginLoc())) return;

        QualType DestType = Cast->getType();
        QualType SrcType  = Cast->getSubExpr()->getType();

        // Exception 1: casting any pointer to void*, char*, unsigned char*, std::byte*
        if (DestType->isPointerType()) {
            QualType Pointee = DestType->getPointeeType().getUnqualifiedType();
            if (Pointee->isVoidType() || Pointee->isCharType() ||
                Pointee->isSpecificBuiltinType(BuiltinType::UChar)) return;
            // std::byte check
            if (Pointee.getAsString() == "std::byte") return;
        }

        // Exception 2: casting pointer to uintptr_t
        if (DestType->isIntegerType() && SrcType->isPointerType()) return;

        auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAI.2.5] reinterpret_cast shall not be used\n";
    }
};

static ReinterpretCastCallback Callback;

} // namespace

void registerReinterpretCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxReinterpretCastExpr().bind("reintCast"),
        &Callback
    );
}

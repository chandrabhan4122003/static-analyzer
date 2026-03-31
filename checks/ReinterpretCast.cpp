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
        if (DestType->isPointerType()) {
            QualType Pointee = DestType->getPointeeType().getUnqualifiedType();
            if (Pointee->isVoidType() || Pointee->isCharType() ||
                Pointee->isSpecificBuiltinType(BuiltinType::UChar)) return;
            if (Pointee.getAsString() == "std::byte") return;
        }
        if (DestType->isIntegerType() && SrcType->isPointerType()) return;
        auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAI.2.5] reinterpret_cast between unrelated types."
                     << " This is almost always undefined behaviour."
                     << " Only allowed when casting to byte/char pointer or uintptr_t.\n";
    }
};
static ReinterpretCastCallback Callback;
} // namespace

void registerReinterpretCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(cxxReinterpretCastExpr().bind("reintCast"), &Callback);
}

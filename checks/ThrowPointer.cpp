#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class ThrowPointerCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Throw = Result.Nodes.getNodeAs<CXXThrowExpr>("throwPtr");
        if (!Throw) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Throw->getBeginLoc())) return;
        auto Loc = SM.getPresumedLoc(Throw->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAS.1.1] Throwing a pointer as an exception is unsafe."
                     << " It is unclear who is responsible for deleting it."
                     << " Throw by value instead: throw MyException(\"message\");\n";
    }
};
static ThrowPointerCallback Callback;
} // namespace

void registerThrowPointerCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxThrowExpr(has(expr(hasType(pointerType())))).bind("throwPtr"),
        &Callback
    );
}

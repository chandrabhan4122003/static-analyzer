#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class UnusedReturnValueCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Call = Result.Nodes.getNodeAs<CallExpr>("unusedCall");
        if (!Call) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Call->getBeginLoc())) return;
        auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
        if (!Loc.isValid()) return;
        std::string funcName = "unknown";
        if (const auto *FD = Call->getDirectCallee())
            if (FD->getDeclName().isIdentifier())
                funcName = FD->getName().str();
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAA.1.2] Return value of '" << funcName
                     << "()' is silently discarded. If intentional, use (void)"
                     << funcName << "() to make it explicit.\n";
    }
};
static UnusedReturnValueCallback Callback;
} // namespace

void registerUnusedReturnValueCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        callExpr(
            hasParent(compoundStmt()),
            unless(hasType(voidType())),
            unless(cxxOperatorCallExpr())
        ).bind("unusedCall"),
        &Callback
    );
}

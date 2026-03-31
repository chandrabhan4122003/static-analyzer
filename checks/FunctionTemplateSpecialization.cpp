#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclTemplate.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class FunctionTemplateSpecializationCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("funcSpecialization");
        if (!FD) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(FD->getBeginLoc())) return;
        auto Loc = SM.getPresumedLoc(FD->getBeginLoc());
        if (!Loc.isValid()) return;
        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAR.8.1] Don't explicitly specialise function template '"
                     << FD->getName() << "'. Specialisations can be silently skipped"
                     << " during overload resolution. Use a regular overload instead.\n";
    }
};
static FunctionTemplateSpecializationCallback Callback;
} // namespace

void registerFunctionTemplateSpecializationCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        functionDecl(isExplicitTemplateSpecialization()).bind("funcSpecialization"),
        &Callback
    );
}

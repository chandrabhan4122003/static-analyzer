#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class DependentBaseLookupCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Call = Result.Nodes.getNodeAs<CallExpr>("unqualifiedCall");
        if (!Call) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Call->getBeginLoc())) return;

        // Must be inside a template class that has a dependent base
        const auto *FD = Result.Nodes.getNodeAs<FunctionDecl>("templateMethod");
        if (!FD) return;

        auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAG.4.3] Unqualified name lookup in template with"
                     << " dependent base; use this-> or qualified name\n";
    }
};

static DependentBaseLookupCallback Callback;

} // namespace

void registerDependentBaseLookupCheck(MatchFinder &Finder) {
    // Match unqualified calls inside template methods that have dependent bases
    Finder.addMatcher(
        callExpr(
            // not a member call (unqualified)
            unless(cxxMemberCallExpr()),
            // not a qualified call
            callee(functionDecl(unless(hasParent(cxxRecordDecl())))),
            // inside a method of a class template
            hasAncestor(cxxMethodDecl(
                ofClass(cxxRecordDecl(hasAncestor(classTemplateDecl())))
            ).bind("templateMethod"))
        ).bind("unqualifiedCall"),
        &Callback
    );
}

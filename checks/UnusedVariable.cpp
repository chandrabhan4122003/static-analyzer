#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class UnusedVariableCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Var = Result.Nodes.getNodeAs<VarDecl>("unusedVar");
        if (!Var) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Var->getBeginLoc())) return;

        // Skip if marked [[maybe_unused]]
        if (Var->hasAttr<UnusedAttr>()) return;

        // Skip if referenced
        if (Var->isReferenced()) return;

        // Skip if has user-provided constructor/destructor (RAII)
        if (const auto *RD = Var->getType()->getAsCXXRecordDecl()) {
            if (RD->hasUserProvidedDefaultConstructor() || !RD->hasTrivialDestructor())
                return;
        }

        auto Loc = SM.getPresumedLoc(Var->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAA.2.1] Variable '" << Var->getName()
                     << "' has limited visibility but is never used\n";
    }
};

static UnusedVariableCallback Callback;

} // namespace

void registerUnusedVariableCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        varDecl(
            anyOf(hasLocalStorage(), isStaticLocal()),
            unless(hasAttr(attr::Unused)),
            unless(parmVarDecl())
        ).bind("unusedVar"),
        &Callback
    );
}

#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class UnusedTypeCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *TD = Result.Nodes.getNodeAs<TagDecl>("unusedType");
        if (!TD) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(TD->getBeginLoc())) return;

        if (TD->hasAttr<UnusedAttr>()) return;

        // Skip if referenced
        if (TD->isReferenced()) return;

        auto Loc = SM.getPresumedLoc(TD->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAA.2.3] Type '" << TD->getName()
                     << "' has limited visibility but is never used\n";
    }
};

static UnusedTypeCallback Callback;

} // namespace

void registerUnusedTypeCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        tagDecl(
            anyOf(isStruct(), isClass(), isEnum()),
            unless(hasAttr(attr::Unused)),
            anyOf(hasParent(compoundStmt()), hasParent(decl()))
        ).bind("unusedType"),
        &Callback
    );
}

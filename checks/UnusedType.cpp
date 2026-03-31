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

        // Safe guard: skip if this is a CXXRecordDecl with bases or virtuals
        if (const auto *RD = dyn_cast<CXXRecordDecl>(TD)) {
            if (!RD->hasDefinition()) return;
            if (RD->getNumBases() > 0) return;
            if (RD->isPolymorphic()) return;
        }

        if (TD->isReferenced()) return;

        // Only flag block-scope types
        if (!TD->getDeclContext()->isFunctionOrMethod()) return;

        auto Loc = SM.getPresumedLoc(TD->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAA.2.3] Type '" << TD->getName()
                     << "' is defined but never used. Remove it or"
                     << " mark [[maybe_unused]] if intentional.\n";
    }
};
static UnusedTypeCallback Callback;
} // namespace

void registerUnusedTypeCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        tagDecl(
            anyOf(isStruct(), isClass(), isEnum()),
            unless(hasAttr(attr::Unused))
        ).bind("unusedType"),
        &Callback
    );
}

#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class ExplicitBaseInitCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Ctor = Result.Nodes.getNodeAs<CXXConstructorDecl>("ctor");
        if (!Ctor || !Ctor->doesThisDeclarationHaveABody()) return;
        if (Ctor->isDelegatingConstructor()) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Ctor->getBeginLoc())) return;

        const auto *Class = Ctor->getParent();

        for (const auto &Base : Class->bases()) {
            const auto *BaseDecl = Base.getType()->getAsCXXRecordDecl();
            if (!BaseDecl) continue;
            if (BaseDecl->isEmpty()) continue;

            bool found = false;
            for (const auto *Init : Ctor->inits()) {
                if (Init->isBaseInitializer() && Init->isWritten()) {
                    if (Init->getBaseClass() &&
                        Init->getBaseClass()->getAsCXXRecordDecl() == BaseDecl) {
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                auto Loc = SM.getPresumedLoc(Ctor->getBeginLoc());
                if (!Loc.isValid()) return;
                // Guard getName()
                std::string baseName = BaseDecl->getDeclName().isIdentifier()
                    ? BaseDecl->getName().str() : "<unnamed>";
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAP.1.2] Constructor does not explicitly"
                             << " initialise base class '" << baseName << "'\n";
            }
        }
    }
};

static ExplicitBaseInitCallback Callback;

} // namespace

void registerExplicitBaseInitCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxConstructorDecl(isDefinition()).bind("ctor"),
        &Callback
    );
}

#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class UninitializedMemberCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Ctor = Result.Nodes.getNodeAs<CXXConstructorDecl>("uninitCtor");
        if (!Ctor || !Ctor->doesThisDeclarationHaveABody()) return;
        if (Ctor->isDelegatingConstructor()) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Ctor->getBeginLoc())) return;
        const auto *Class = Ctor->getParent();
        for (const auto *Field : Class->fields()) {
            if (Field->hasInClassInitializer()) continue;
            if (!Field->getDeclName().isIdentifier()) continue;
            bool initialized = false;
            for (const auto *Init : Ctor->inits()) {
                if (Init->getMember() == Field && Init->isWritten()) {
                    initialized = true;
                    break;
                }
            }
            if (!initialized) {
                auto Loc = SM.getPresumedLoc(Ctor->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAP.1.4] Member '" << Field->getName()
                             << "' is not initialised in the constructor initialiser"
                             << " list. Reading it before assignment is undefined"
                             << " behaviour. Add it to the initialiser list or"
                             << " give it a default member initialiser.\n";
            }
        }
    }
};
static UninitializedMemberCallback Callback;
} // namespace

void registerUninitializedMemberCheck(MatchFinder &Finder) {
    Finder.addMatcher(cxxConstructorDecl(isDefinition()).bind("uninitCtor"), &Callback);
}

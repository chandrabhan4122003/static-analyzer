#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class VirtualSpecifierCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *MD = Result.Nodes.getNodeAs<CXXMethodDecl>("method");
        if (!MD) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(MD->getBeginLoc())) return;

        // Guard against non-identifier names (operators, destructors)
        if (!MD->getDeclName().isIdentifier()) return;

        bool isVirt  = MD->isVirtualAsWritten();
        bool isOvrd  = MD->hasAttr<OverrideAttr>();
        bool isFnl   = MD->hasAttr<FinalAttr>();
        bool isOvrdFn = MD->size_overridden_methods() > 0;

        auto Loc = SM.getPresumedLoc(MD->getBeginLoc());
        if (!Loc.isValid()) return;

        if (isVirt && isOvrd) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] Redundant 'virtual' and 'override' on '"
                         << MD->getName() << "'\n";
            return;
        }
        if (isOvrd && isFnl) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] Redundant 'override' and 'final' on '"
                         << MD->getName() << "'\n";
            return;
        }
        if (isVirt && isFnl) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] Redundant 'virtual' and 'final' on '"
                         << MD->getName() << "'\n";
            return;
        }
        if (isOvrdFn && !isOvrd && !isFnl) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] Overriding function '" << MD->getName()
                         << "' missing 'override' or 'final' specifier\n";
        }
    }
};

static VirtualSpecifierCallback Callback;

} // namespace

void registerVirtualSpecifierCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxMethodDecl(
            isDefinition(),
            unless(cxxConstructorDecl()),
            unless(cxxDestructorDecl())
        ).bind("method"),
        &Callback
    );
}

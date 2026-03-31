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
        if (!MD->getDeclName().isIdentifier()) return;
        bool isVirt  = MD->isVirtualAsWritten();
        bool isOvrd  = MD->hasAttr<OverrideAttr>();
        bool isFnl   = MD->hasAttr<FinalAttr>();
        bool isOvrdFn = MD->size_overridden_methods() > 0;
        auto Loc = SM.getPresumedLoc(MD->getBeginLoc());
        if (!Loc.isValid()) return;
        if (isVirt && isOvrd) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] '" << MD->getName()
                         << "' has both 'virtual' and 'override'. 'override' already"
                         << " implies virtual. Remove 'virtual'.\n";
            return;
        }
        if (isOvrd && isFnl) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] '" << MD->getName()
                         << "' has both 'override' and 'final'. 'final' already"
                         << " implies override. Remove 'override'.\n";
            return;
        }
        if (isVirt && isFnl) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] '" << MD->getName()
                         << "' has both 'virtual' and 'final'. Use only 'final'.\n";
            return;
        }
        if (isOvrdFn && !isOvrd && !isFnl) {
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAN.3.1] '" << MD->getName()
                         << "' overrides a base class function but is missing"
                         << " 'override' specifier. Add 'override' to make"
                         << " the intent clear and catch signature mismatches.\n";
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

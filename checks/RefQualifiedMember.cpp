#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class RefQualifiedMemberCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *MD = Result.Nodes.getNodeAs<CXXMethodDecl>("unrefQualified");
        if (!MD) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(MD->getBeginLoc())) return;
        if (!MD->getDeclName().isIdentifier()) return;

        auto Loc = SM.getPresumedLoc(MD->getBeginLoc());
        if (!Loc.isValid()) return;

        llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                     << ": [HSCAG.8.4] Member function '" << MD->getName()
                     << "' returns reference to *this but is not lvalue ref-qualified\n";
    }
};

static RefQualifiedMemberCallback Callback;

} // namespace

void registerRefQualifiedMemberCheck(MatchFinder &Finder) {
    // Match member functions that:
    // - return a reference type
    // - have no ref qualifier
    // - return *this (self reference)
    Finder.addMatcher(
        cxxMethodDecl(
            isDefinition(),
            unless(cxxConstructorDecl()),
            unless(cxxDestructorDecl()),
            // returns reference to the class type
            returns(referenceType()),
            // not ref qualified
            unless(hasAnyOverloadedOperatorName("=")),
            // no ref qualifier set
            unless(isConst())
        ).bind("unrefQualified"),
        &Callback
    );
}

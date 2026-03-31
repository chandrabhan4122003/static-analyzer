#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class ExplicitConstructorCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;

        if (const auto *Ctor = Result.Nodes.getNodeAs<CXXConstructorDecl>("implicitCtor")) {
            if (!SM.isInMainFile(Ctor->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Ctor->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.1.3] Constructor callable with single argument"
                         << " shall be explicit\n";
            return;
        }

        if (const auto *Conv = Result.Nodes.getNodeAs<CXXConversionDecl>("implicitConv")) {
            if (!SM.isInMainFile(Conv->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Conv->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.1.3] Conversion operator shall be explicit\n";
        }
    }
};

static ExplicitConstructorCallback Callback;

} // namespace

void registerExplicitConstructorCheck(MatchFinder &Finder) {
    // Single argument constructor not marked explicit
    Finder.addMatcher(
        cxxConstructorDecl(
            unless(isExplicit()),
            unless(isCopyConstructor()),
            unless(isMoveConstructor()),
            parameterCountIs(1)
        ).bind("implicitCtor"),
        &Callback
    );
    // Constructor with defaults making it callable with one arg
    Finder.addMatcher(
        cxxConstructorDecl(
            unless(isExplicit()),
            unless(isCopyConstructor()),
            unless(isMoveConstructor()),
            hasParameter(0, parmVarDecl()),
            unless(parameterCountIs(1))
        ).bind("implicitCtor"),
        &Callback
    );
    // Conversion operator not marked explicit
    Finder.addMatcher(
        cxxConversionDecl(
            unless(isExplicit())
        ).bind("implicitConv"),
        &Callback
    );
}

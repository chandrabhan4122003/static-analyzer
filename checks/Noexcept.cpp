#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

bool isNoexcept(const FunctionDecl *FD) {
    const auto *FPT = FD->getType()->getAs<FunctionProtoType>();
    if (!FPT) return false;
    return FPT->isNothrow();
}

class NoexceptCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;

        // Destructor
        if (const auto *DD = Result.Nodes.getNodeAs<CXXDestructorDecl>("dtor")) {
            if (!SM.isInMainFile(DD->getBeginLoc())) return;
            if (!isNoexcept(DD)) {
                auto Loc = SM.getPresumedLoc(DD->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Destructor shall be noexcept\n";
            }
            return;
        }

        // Move constructor
        if (const auto *MC = Result.Nodes.getNodeAs<CXXConstructorDecl>("moveCtor")) {
            if (!SM.isInMainFile(MC->getBeginLoc())) return;
            if (!isNoexcept(MC)) {
                auto Loc = SM.getPresumedLoc(MC->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Move constructor shall be noexcept\n";
            }
            return;
        }

        // Move assignment
        if (const auto *MA = Result.Nodes.getNodeAs<CXXMethodDecl>("moveAssign")) {
            if (!SM.isInMainFile(MA->getBeginLoc())) return;
            if (!isNoexcept(MA)) {
                auto Loc = SM.getPresumedLoc(MA->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Move assignment operator shall be noexcept\n";
            }
            return;
        }

        // Swap function
        if (const auto *SW = Result.Nodes.getNodeAs<FunctionDecl>("swapFunc")) {
            if (!SM.isInMainFile(SW->getBeginLoc())) return;
            if (!isNoexcept(SW)) {
                auto Loc = SM.getPresumedLoc(SW->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Function named 'swap' shall be noexcept\n";
            }
        }
    }
};

static NoexceptCallback Callback;

} // namespace

void registerNoexceptCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxDestructorDecl(isDefinition()).bind("dtor"),
        &Callback
    );
    Finder.addMatcher(
        cxxConstructorDecl(isMoveConstructor(), isDefinition()).bind("moveCtor"),
        &Callback
    );
    Finder.addMatcher(
        cxxMethodDecl(isMoveAssignmentOperator(), isDefinition()).bind("moveAssign"),
        &Callback
    );
    Finder.addMatcher(
        functionDecl(hasName("swap"), isDefinition()).bind("swapFunc"),
        &Callback
    );
}

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

        if (const auto *DD = Result.Nodes.getNodeAs<CXXDestructorDecl>("dtor")) {
            if (!SM.isInMainFile(DD->getBeginLoc())) return;
            if (!DD->doesThisDeclarationHaveABody()) return;
            if (DD->isImplicit()) return;
            if (!isNoexcept(DD)) {
                auto Loc = SM.getPresumedLoc(DD->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Destructor is not noexcept."
                             << " If it throws during stack unwinding the program"
                             << " terminates. Mark it noexcept.\n";
            }
            return;
        }

        if (const auto *MC = Result.Nodes.getNodeAs<CXXConstructorDecl>("moveCtor")) {
            if (!SM.isInMainFile(MC->getBeginLoc())) return;
            if (!MC->doesThisDeclarationHaveABody()) return;
            if (MC->isImplicit()) return;
            if (!isNoexcept(MC)) {
                auto Loc = SM.getPresumedLoc(MC->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Move constructor is not noexcept."
                             << " std containers can't use it safely. Mark it noexcept.\n";
            }
            return;
        }

        if (const auto *MA = Result.Nodes.getNodeAs<CXXMethodDecl>("moveAssign")) {
            if (!SM.isInMainFile(MA->getBeginLoc())) return;
            if (!MA->doesThisDeclarationHaveABody()) return;
            if (MA->isImplicit()) return;
            if (!isNoexcept(MA)) {
                auto Loc = SM.getPresumedLoc(MA->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] Move assignment operator is not noexcept."
                             << " Mark it noexcept to enable safe use in std containers.\n";
            }
            return;
        }

        if (const auto *SW = Result.Nodes.getNodeAs<FunctionDecl>("swapFunc")) {
            if (!SM.isInMainFile(SW->getBeginLoc())) return;
            if (!SW->doesThisDeclarationHaveABody()) return;
            if (!isNoexcept(SW)) {
                auto Loc = SM.getPresumedLoc(SW->getBeginLoc());
                if (!Loc.isValid()) return;
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAS.4.1] swap() is not noexcept."
                             << " swap() should never throw. Mark it noexcept.\n";
            }
        }
    }
};
static NoexceptCallback Callback;
} // namespace

void registerNoexceptCheck(MatchFinder &Finder) {
    Finder.addMatcher(cxxDestructorDecl(isDefinition(), unless(isImplicit())).bind("dtor"), &Callback);
    Finder.addMatcher(cxxConstructorDecl(isMoveConstructor(), isDefinition(), unless(isImplicit())).bind("moveCtor"), &Callback);
    Finder.addMatcher(cxxMethodDecl(isMoveAssignmentOperator(), isDefinition(), unless(isImplicit())).bind("moveAssign"), &Callback);
    Finder.addMatcher(functionDecl(hasName("swap"), isDefinition()).bind("swapFunc"), &Callback);
}

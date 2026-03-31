#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class HiddenFunctionCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *MD = Result.Nodes.getNodeAs<CXXMethodDecl>("hiddenMethod");
        if (!MD) return;
        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(MD->getBeginLoc())) return;
        if (MD->isVirtual()) return;
        if (!MD->getDeclName().isIdentifier()) return;
        const auto *Class = MD->getParent();
        for (const auto &Base : Class->bases()) {
            if (Base.getAccessSpecifier() == AS_private) continue;
            const auto *BaseDecl = Base.getType()->getAsCXXRecordDecl();
            if (!BaseDecl) continue;
            for (const auto *BaseMethod : BaseDecl->methods()) {
                if (!BaseMethod->getDeclName().isIdentifier()) continue;
                if (BaseMethod->getDeclName() == MD->getDeclName() &&
                    !BaseMethod->isVirtual() &&
                    BaseMethod->getAccess() != AS_private) {
                    auto Loc = SM.getPresumedLoc(MD->getBeginLoc());
                    if (!Loc.isValid()) return;
                    llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                                 << ": [HSCAG.4.2] '" << MD->getName()
                                 << "' hides a base class function with the same name."
                                 << " Callers may get the wrong function."
                                 << " Add 'using Base::" << MD->getName()
                                 << ";' to bring the base overloads into scope.\n";
                    return;
                }
            }
        }
    }
};
static HiddenFunctionCallback Callback;
} // namespace

void registerHiddenFunctionCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxMethodDecl(
            isDefinition(),
            unless(isOverride()),
            unless(cxxConstructorDecl()),
            unless(cxxDestructorDecl())
        ).bind("hiddenMethod"),
        &Callback
    );
}

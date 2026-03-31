#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class DynamicMemoryCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;
        if (const auto *NE = Result.Nodes.getNodeAs<CXXNewExpr>("newExpr")) {
            if (!SM.isInMainFile(NE->getBeginLoc())) return;
            if (NE->getNumPlacementArgs() > 0) return;
            auto Loc = SM.getPresumedLoc(NE->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.2] Raw 'new' used. Manual memory management"
                         << " risks leaks and double-frees."
                         << " Use std::make_unique<T>() or std::make_shared<T>()"
                         << " instead.\n";
            return;
        }
        if (const auto *DE = Result.Nodes.getNodeAs<CXXDeleteExpr>("deleteExpr")) {
            if (!SM.isInMainFile(DE->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(DE->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.2] Raw 'delete' used. Let smart pointers"
                         << " (unique_ptr/shared_ptr) manage object lifetime"
                         << " automatically.\n";
            return;
        }
        if (const auto *Call = Result.Nodes.getNodeAs<CallExpr>("mallocCall")) {
            if (!SM.isInMainFile(Call->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Call->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAV.6.2] C-style memory function used."
                         << " Use std::vector or smart pointers for automatic"
                         << " memory management.\n";
        }
    }
};
static DynamicMemoryCallback Callback;
} // namespace

void registerDynamicMemoryCheck(MatchFinder &Finder) {
    Finder.addMatcher(cxxNewExpr().bind("newExpr"), &Callback);
    Finder.addMatcher(cxxDeleteExpr().bind("deleteExpr"), &Callback);
    Finder.addMatcher(
        callExpr(callee(functionDecl(anyOf(
            hasName("malloc"), hasName("free"),
            hasName("calloc"), hasName("realloc"),
            hasName("aligned_alloc")
        )))).bind("mallocCall"),
        &Callback
    );
}

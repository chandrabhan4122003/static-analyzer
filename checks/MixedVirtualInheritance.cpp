#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class MixedVirtualInheritanceCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *RD = Result.Nodes.getNodeAs<CXXRecordDecl>("classDecl");
        if (!RD || !RD->hasDefinition()) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(RD->getBeginLoc())) return;

        // Collect all base classes reachable both virtually and non-virtually
        llvm::SmallSet<const CXXRecordDecl*, 8> virtualBases;
        llvm::SmallSet<const CXXRecordDecl*, 8> nonVirtualBases;

        collectBases(RD, virtualBases, nonVirtualBases);

        for (const auto *Base : virtualBases) {
            if (nonVirtualBases.count(Base)) {
                auto Loc = SM.getPresumedLoc(RD->getBeginLoc());
                if (!Loc.isValid()) return;
                std::string baseName = Base->getDeclName().isIdentifier()
                    ? Base->getName().str() : "<unnamed>";
                llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                             << ": [HSCAN.1.2] Base class '" << baseName
                             << "' is inherited both virtually and non-virtually\n";
                return;
            }
        }
    }

private:
    void collectBases(const CXXRecordDecl *RD,
                      llvm::SmallSet<const CXXRecordDecl*, 8> &virt,
                      llvm::SmallSet<const CXXRecordDecl*, 8> &nonVirt) {
        for (const auto &Base : RD->bases()) {
            const auto *BaseDecl = Base.getType()->getAsCXXRecordDecl();
            if (!BaseDecl) continue;
            if (Base.isVirtual()) virt.insert(BaseDecl);
            else nonVirt.insert(BaseDecl);
            collectBases(BaseDecl, virt, nonVirt);
        }
    }
};

static MixedVirtualInheritanceCallback Callback;

} // namespace

void registerMixedVirtualInheritanceCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxRecordDecl(isDefinition()).bind("classDecl"),
        &Callback
    );
}

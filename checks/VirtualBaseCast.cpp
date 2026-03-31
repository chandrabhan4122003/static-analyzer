#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class VirtualBaseCastCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        auto &SM = *Result.SourceManager;

        if (const auto *Cast = Result.Nodes.getNodeAs<CXXStaticCastExpr>("staticFromVirtual")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.2.1] static_cast from virtual base class;"
                         << " use dynamic_cast instead\n";
            return;
        }

        if (const auto *Cast = Result.Nodes.getNodeAs<CXXReinterpretCastExpr>("reintFromVirtual")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.2.1] reinterpret_cast from virtual base class;"
                         << " use dynamic_cast instead\n";
        }
    }

    // Helper to check if source type is a virtual base of dest type
    static bool isVirtualBase(const CXXRecordDecl *Derived, const CXXRecordDecl *Base) {
        if (!Derived || !Base) return false;
        for (const auto &B : Derived->bases()) {
            if (B.isVirtual()) {
                const auto *BD = B.getType()->getAsCXXRecordDecl();
                if (BD == Base) return true;
            }
        }
        return false;
    }
};

static VirtualBaseCastCallback Callback;

} // namespace

void registerVirtualBaseCastCheck(MatchFinder &Finder) {
    // static_cast where source is a virtual base
    Finder.addMatcher(
        cxxStaticCastExpr(
            hasSourceExpression(expr(hasType(
                pointsTo(cxxRecordDecl().bind("srcRecord"))
            ))),
            hasDestinationType(pointsTo(cxxRecordDecl()))
        ).bind("staticFromVirtual"),
        &Callback
    );
    Finder.addMatcher(
        cxxReinterpretCastExpr(
            hasSourceExpression(expr(hasType(
                pointsTo(cxxRecordDecl())
            ))),
            hasDestinationType(pointsTo(cxxRecordDecl()))
        ).bind("reintFromVirtual"),
        &Callback
    );
}

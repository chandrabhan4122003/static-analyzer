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
                         << ": [HSCAI.2.1] Don't use static_cast to cast from a"
                         << " virtual base class. Use dynamic_cast instead.\n";
            return;
        }
        if (const auto *Cast = Result.Nodes.getNodeAs<CXXReinterpretCastExpr>("reintFromVirtual")) {
            if (!SM.isInMainFile(Cast->getBeginLoc())) return;
            auto Loc = SM.getPresumedLoc(Cast->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAI.2.1] Don't use reinterpret_cast from a virtual"
                         << " base class. Use dynamic_cast instead.\n";
        }
    }
};
static VirtualBaseCastCallback Callback;
} // namespace

void registerVirtualBaseCastCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxStaticCastExpr(
            hasSourceExpression(expr(hasType(pointsTo(cxxRecordDecl())))),
            hasDestinationType(pointsTo(cxxRecordDecl()))
        ).bind("staticFromVirtual"),
        &Callback
    );
    Finder.addMatcher(
        cxxReinterpretCastExpr(
            hasSourceExpression(expr(hasType(pointsTo(cxxRecordDecl())))),
            hasDestinationType(pointsTo(cxxRecordDecl()))
        ).bind("reintFromVirtual"),
        &Callback
    );
}

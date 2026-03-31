#include "Checks.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/DeclCXX.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {

class SelfAssignmentCallback : public MatchFinder::MatchCallback {
public:
    void run(const MatchFinder::MatchResult &Result) override {
        const auto *Op = Result.Nodes.getNodeAs<CXXMethodDecl>("assignOp");
        if (!Op || !Op->doesThisDeclarationHaveABody()) return;

        auto &SM = *Result.SourceManager;
        if (!SM.isInMainFile(Op->getBeginLoc())) return;

        const auto *Body = dyn_cast<CompoundStmt>(Op->getBody());
        if (!Body) return;

        bool hasSelfCheck = false;
        for (const auto *S : Body->body()) {
            std::string text;
            llvm::raw_string_ostream OS(text);
            S->printPretty(OS, nullptr,
                PrintingPolicy(Op->getASTContext().getLangOpts()));
            OS.flush();
            if (text.find("this") != std::string::npos &&
                (text.find("!=") != std::string::npos ||
                 text.find("addressof") != std::string::npos ||
                 text.find("swap") != std::string::npos)) {
                hasSelfCheck = true;
                break;
            }
        }

        if (!hasSelfCheck) {
            auto Loc = SM.getPresumedLoc(Op->getBeginLoc());
            if (!Loc.isValid()) return;
            llvm::errs() << Loc.getFilename() << ":" << Loc.getLine()
                         << ": [HSCAP.0.2] Assignment operator does not handle"
                         << " self-assignment\n";
        }
    }
};

static SelfAssignmentCallback Callback;

} // namespace

void registerSelfAssignmentCheck(MatchFinder &Finder) {
    Finder.addMatcher(
        cxxMethodDecl(
            anyOf(isCopyAssignmentOperator(), isMoveAssignmentOperator()),
            isDefinition()
        ).bind("assignOp"),
        &Callback
    );
}

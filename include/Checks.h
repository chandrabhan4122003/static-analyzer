#pragma once
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Tooling.h"

// HSCAA
void registerUnusedReturnValueCheck(clang::ast_matchers::MatchFinder &Finder);
void registerUnusedVariableCheck(clang::ast_matchers::MatchFinder &Finder);
void registerUnusedParameterCheck(clang::ast_matchers::MatchFinder &Finder);
void registerUnusedTypeCheck(clang::ast_matchers::MatchFinder &Finder);
void registerUnusedFunctionCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAG
void registerHiddenFunctionCheck(clang::ast_matchers::MatchFinder &Finder);
void registerDanglingAssignmentCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAP
void registerDynamicTypeInCtorDtorCheck(clang::ast_matchers::MatchFinder &Finder);
void registerExplicitBaseInitCheck(clang::ast_matchers::MatchFinder &Finder);
void registerExplicitConstructorCheck(clang::ast_matchers::MatchFinder &Finder);
void registerUninitializedMemberCheck(clang::ast_matchers::MatchFinder &Finder);
void registerSelfAssignmentCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAN
void registerVirtualSpecifierCheck(clang::ast_matchers::MatchFinder &Finder);
void registerDefaultArgOverrideCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAR
void registerFunctionTemplateSpecializationCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAS
void registerThrowPointerCheck(clang::ast_matchers::MatchFinder &Finder);
void registerNoexceptCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAI
void registerCStyleCastCheck(clang::ast_matchers::MatchFinder &Finder);
void registerConstCastCheck(clang::ast_matchers::MatchFinder &Finder);
void registerReinterpretCastCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAJ
void registerMissingElseCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCAV
void registerDynamicMemoryCheck(clang::ast_matchers::MatchFinder &Finder);

// HSCBC
void registerMovedFromStateCheck(clang::ast_matchers::MatchFinder &Finder);

// Additional checks
void registerVirtualBaseCastCheck(clang::ast_matchers::MatchFinder &Finder);
void registerIntToPointerCastCheck(clang::ast_matchers::MatchFinder &Finder);
void registerOverlappingCopyCheck(clang::ast_matchers::MatchFinder &Finder);
void registerAdvancedMemoryManagementCheck(clang::ast_matchers::MatchFinder &Finder);
void registerMixedVirtualInheritanceCheck(clang::ast_matchers::MatchFinder &Finder);
void registerVirtualMemberPtrComparisonCheck(clang::ast_matchers::MatchFinder &Finder);
void registerDependentBaseLookupCheck(clang::ast_matchers::MatchFinder &Finder);
void registerRefQualifiedMemberCheck(clang::ast_matchers::MatchFinder &Finder);

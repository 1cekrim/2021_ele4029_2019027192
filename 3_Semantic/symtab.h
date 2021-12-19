/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"

/* SIZE is the size of the hash table */
#define SIZE 211
/* the list of line numbers of the source
 * code in which a variable is referenced
 */
typedef struct LineListRec
{
    int lineno;
    struct LineListRec* next;
} * LineList;

typedef struct FunctionArgsListRec
{
    char* name; // null possible
    ExpType type;
    int isarray;
    struct FunctionArgsListRec* next;
}* FunctionArgsList;

typedef struct
{
    int args_count;
    FunctionArgsList args;
} FunctionInfo;

typedef struct BucketListRec
{
    char* name;
    ExpType type;
    int isarray;
    SymbolKind kind;
    LineList lines;
    int memloc;
    struct BucketListRec* next;
    FunctionInfo functionInfo
}* BucketList;


typedef struct ScopeListRec
{
    char* name;
    BucketList bucket[SIZE];
    struct ScopeListRec* parent;
    struct ScopeListRec* leftmost;
    struct ScopeListRec* sibling;
}* ScopeList;

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
BucketList st_insert(ScopeList scope, char* name, ExpType type, int isarray, SymbolKind kind, int lineno, int loc);
int st_insert_lineno(ScopeList scope, char* name, int lineno);

/* Function st_lookup returns the memory
 * location of a variable or -1 if not found
 */
BucketList st_lookup(ScopeList scope, char* name);
BucketList st_lookup_excluding_parent(ScopeList scope, char* name);

ScopeList create_ScopeList(ScopeList parent, char* name);

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */
void printSymTab(FILE* listing, ScopeList root);
void printFuncTab(FILE* listing, ScopeList root);

void addFuncArg(BucketList func, TreeNode* param);

#endif

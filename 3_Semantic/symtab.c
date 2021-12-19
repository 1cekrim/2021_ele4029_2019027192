/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "util.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash(char* key)
{
    int temp = 0;
    int i = 0;
    while (key[i] != '\0')
    {
        temp = ((temp << SHIFT) + key[i]) % SIZE;
        ++i;
    }
    return temp;
}

ScopeList create_ScopeList(ScopeList parent, char* name)
{
    ScopeList scope = (ScopeList)malloc(sizeof(struct ScopeListRec));
    memset(scope, 0, sizeof(struct ScopeListRec));
    scope->name = copyString(name);
    scope->parent = parent;
    if (parent)
    {
        ScopeList* target = &parent->leftmost;
        while (*target)
        {
            target = &(*target)->sibling;
        }
        *target = scope;
    }
    return scope;
}

/* Success: return BucketList, Failure(redefine): return NULL */
BucketList st_insert(ScopeList scope, char* name, ExpType type, int isarray, SymbolKind kind, int lineno, int loc)
{
    int h = hash(name);
    BucketList l = st_lookup_excluding_parent(scope, name);
    if (l)
    {
        return NULL;
    }

    l = (BucketList)malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList)malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->type = type;
    l->kind = kind;
    l->isarray = isarray;
    l->lines->next = NULL;
    l->next = scope->bucket[h];
    l->functionInfo.args = NULL;
    scope->bucket[h] = l;
    return l;
} /* st_insert */

/* Success: return 0, Failure(undefined): return -1 */
int st_insert_lineno(ScopeList scope, char* name, int lineno)
{
    int h = hash(name);
    BucketList l = st_lookup(scope, name);
    if (!l)
    {
        return -1;
    }

    LineList t = l->lines;
    while (t->next != NULL)
        t = t->next;
    t->next = (LineList)malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
    return 0;
}

/* Function st_lookup returns the memory
 * location of a variable or -1 if not found
 */
BucketList st_lookup(ScopeList scope, char* name)
{
    while (scope)
    {
        int h = hash(name);
        BucketList l = scope->bucket[h];
        while ((l != NULL) && (strcmp(name, l->name) != 0))
            l = l->next;
        if (l)
        {
            return l;
        }
        scope = scope->parent;
    }
    return NULL;
}

BucketList st_lookup_excluding_parent(ScopeList scope, char* name)
{
    int h = hash(name);
    BucketList l = scope->bucket[h];
    while ((l != NULL) && (strcmp(name, l->name) != 0))
        l = l->next;
    if (l)
    {
        return l;
    }

    return NULL;
}

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */

const char* variable_type_string[5] = {"Function", "Integer", "Void", "Integer Array", "Invalid Type"};
const char* get_variable_type_string(ExpType type, SymbolKind kind, int isarray)
{
    if (kind == FuncSymbol)
    {
        return variable_type_string[0];
    }
    if (type == Integer && isarray)
    {
        return variable_type_string[3];
    }
    if (type == Integer && !isarray)
    {
        return variable_type_string[1];
    }
    if (type == Void)
    {
        return variable_type_string[2];
    }
    return variable_type_string[4];
}

static void symTabTraverse(FILE* listing, ScopeList now, void (*callback)(FILE*, ScopeList))
{
    if (now != NULL)
    {
        callback(listing, now);
        symTabTraverse(listing, now->sibling, callback);
        symTabTraverse(listing, now->leftmost, callback);
    }
}

void printSymTabCallback(FILE* listing, ScopeList scope)
{
    for (int i = 0; i < SIZE; ++i)
    {
        if (scope->bucket[i] != NULL)
        {
            BucketList l = scope->bucket[i];
            while (l != NULL)
            {
                LineList t = l->lines;
                fprintf(listing, "%-14s ", l->name);
                fprintf(listing, "%-14s ", get_variable_type_string(l->type, l->kind, l->isarray));
                fprintf(listing, "%-11s ", scope->name);
                fprintf(listing, "%-8d ", l->memloc);
                while (t != NULL)
                {
                    fprintf(listing, "%4d ", t->lineno);
                    t = t->next;
                }
                fprintf(listing, "\n");
                l = l->next;
            }
        }
    }
}

void printSymTab(FILE* listing, ScopeList root)
{
    int i;
    fprintf(listing, "\n< Symbol Table >\n");
    fprintf(listing, "Variable Name  Variable Type  Scope Name  Location   Line Numbers\n");
    fprintf(listing, "-------------  -------------  ----------  --------   ------------\n");
    symTabTraverse(listing, root, printSymTabCallback);
} /* printSymTab */

void printFunctionTableCallback(FILE* listing, ScopeList scope)
{
    for (int i = 0; i < SIZE; ++i)
    {
        if (scope->bucket[i] != NULL)
        {
            BucketList l = scope->bucket[i];
            while (l != NULL && l->kind == FuncSymbol)
            {
                fprintf(listing, "%-14s ", l->name);
                fprintf(listing, "%-11s ", scope->name);
                fprintf(listing, "%-11s ", get_variable_type_string(l->type, VarSymbol, l->isarray));
                
                FunctionArgsList arg = l->functionInfo.args;
                if (l->functionInfo.args_count == 0)
                {
                    fprintf(listing, "%-17s Void\n", " ");
                }
                else
                {
                    fprintf(listing, "\n");
                    while (arg)
                    {
                        fprintf(listing, "%-38s  %-16s %s\n", " ", arg->name, get_variable_type_string(arg->type, VarSymbol, arg->isarray));
                        arg = arg->next;
                    }
                }
                
                l = l->next;
            }
        }
    }
}

void printFuncTab(FILE* listing, ScopeList root)
{
    int i;
    fprintf(listing, "\n< Function Table >\n");
    fprintf(listing, "Function Name  Scope Name  Return Type  Parameter Name   Parameter Type\n");
    fprintf(listing, "-------------  ----------  -----------  --------------   --------------\n");
    symTabTraverse(listing, root, printFunctionTableCallback);
} /* printSymTab */


void addFuncArg(BucketList func, TreeNode* param)
{
    FunctionArgsList* arg = &func->functionInfo.args;
    while (*arg)
    {
        arg = &(*arg)->next;
    }
    *arg = (FunctionArgsList)malloc(sizeof(struct FunctionArgsListRec));
    memset(*arg, 0, sizeof(struct FunctionArgsListRec));
    (*arg)->isarray = param->isarray;
    (*arg)->name = param->attr.name;
    (*arg)->type = (*arg)->type;
    ++func->functionInfo.args_count;
}
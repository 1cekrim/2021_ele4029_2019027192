/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

static void typeError(TreeNode* t, char* message)
{
    fprintf(listing, "Type error at line %d: %s\n", t->lineno, message);
    Error = TRUE;
}

static void undeclaredError(TreeNode* t)
{
    fprintf(listing,
            "Undeclared error at line %d: '%s' undeclared\n",
            t->lineno,
            t->attr.name);
    Error = TRUE;
}

static void redeclaredError(TreeNode* t)
{
    fprintf(listing,
            "Redeclared error at line %d: '%s' redeclared\n",
            t->lineno,
            t->attr.name);
    Error = TRUE;
}

static void declarationError(TreeNode* t, char* message)
{
    fprintf(listing, "declaration error at line %d: %s\n", t->lineno, message);
    Error = TRUE;
}

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void
traverse(TreeNode* t, void (*preProc)(TreeNode*), void (*postProc)(TreeNode*))
{
    if (t != NULL)
    {
        preProc(t);
        {
            int i;
            for (i = 0; i < MAXCHILDREN; i++)
                traverse(t->child[i], preProc, postProc);
        }
        postProc(t);
        traverse(t->sibling, preProc, postProc);
    }
}

/* nullProc is a do-nothing procedure to
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode* t)
{
    if (t == NULL)
        return;
    else
        return;
}

typedef struct
{
    ScopeList scope;
    int location;
} ScopeStackPair;

static ScopeStackPair scope_stack[MAXSCOPEDEPTH];
static int scope_stack_top_index = -1;
int scope_stack_push(ScopeList scope, int location)
{
    if (scope_stack_top_index >= MAXSCOPEDEPTH - 1)
    {
        return -1;
    }
    scope_stack[++scope_stack_top_index].scope = scope;
    scope_stack[scope_stack_top_index].location = location;
    return 0;
}

ScopeStackPair* scope_stack_top()
{
    return &scope_stack[scope_stack_top_index];
}

int scope_stack_pop()
{
    if (scope_stack_top_index < 0)
    {
        return -1;
    }
    scope_stack[scope_stack_top_index].scope = NULL;
    scope_stack[scope_stack_top_index--].location = 0;
    return 0;
}

static int built_in_functions(ScopeList scope, int location)
{
    // output
    {
        st_insert(scope, "output", Integer, FALSE, FuncSymbol, 0, location++);
    }

    // input
    {
        BucketList input = st_insert(scope, "input", Integer, FALSE, FuncSymbol, 0, location++);
        TreeNode param;
        param.isarray = FALSE;
        param.attr.name = "";
        param.type = Integer;
        addFuncArg(input, &param);
    }
    return location;
}

static ScopeList init_global_scope()
{
    static ScopeList global_scope;
    static int global_scope_location;
    global_scope = create_ScopeList(NULL, "global");
    global_scope_location = built_in_functions(global_scope, 0);
    scope_stack_push(global_scope, global_scope_location);
    return global_scope;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode(TreeNode* t)
{
    static int is_func_compound = FALSE;
    static BucketList current_function;
    ScopeStackPair* pair = scope_stack_top();
    switch (t->nodekind)
    {
        case ExpK:
            switch (t->kind.exp)
            {
                case AssignmentK:
                case OperatorK:
                case ConstantK:
                    break;

                case CallK:
                case VarK:
                    if (st_insert_lineno(pair->scope, t->attr.name, t->lineno))
                    {
                        undeclaredError(t);
                    }
                    break;
                default:
                    break;
            }
            break;
        case StmtK:
            switch (t->kind.stmt)
            {
                case CompoundK:
                    if (!is_func_compound)
                    {
                        scope_stack_push(
                            create_ScopeList(pair->scope, "compound"), 0);
                        pair->location++;
                    }
                    is_func_compound = FALSE;
                    break;
                case SelectionK:
                case IterationK:
                case ReturnK:
                default:
                    break;
            }
            break;
        case DeclarationK:
            switch (t->kind.declaration)
            {
                case FuncK:
                    if (pair->scope->parent) // not global scope
                    {
                        printf("%s\n", pair->scope->name);
                        declarationError(
                            t,
                            "Functions can only be declared in global scope.");
                        break;
                    }
                    current_function = st_insert(pair->scope,
                                  t->attr.name,
                                  t->type,
                                  t->isarray,
                                  FuncSymbol,
                                  t->lineno,
                                  pair->location++);
                    if (!current_function)
                    {
                        redeclaredError(t);
                    }
                    scope_stack_push(
                        create_ScopeList(pair->scope, t->attr.name), 0);
                    is_func_compound = TRUE;
                    break;
                case VarDeclarationK:
                    if (!st_insert(pair->scope,
                                  t->attr.name,
                                  t->type,
                                  t->isarray,
                                  VarSymbol,
                                  t->lineno,
                                  pair->location++))
                    {
                        redeclaredError(t);
                    }
                    break;
                case ParameterK:
                    if (!st_insert(pair->scope,
                                  t->attr.name,
                                  t->type,
                                  t->isarray,
                                  VarSymbol,
                                  t->lineno,
                                  pair->location++))
                    {
                        redeclaredError(t);
                    }
                    addFuncArg(current_function, t);
                    break;
                case VoidParameterK:
                default:
                    break;
            }
        default:
            break;
    }
}

static void afterInsertNode(const TreeNode* t)
{
    if (t->nodekind == StmtK && t->kind.stmt == CompoundK)
    {
        scope_stack_pop();
    }
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode* syntaxTree)
{
    ScopeList global_scope = init_global_scope();
    traverse(syntaxTree, insertNode, afterInsertNode);
    if (TraceAnalyze)
    {
        printSymTab(listing, global_scope);
        printFuncTab(listing, global_scope);
    }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */

/*
• Type checking for functions and variables.
– The type “void” is only available for functions.
– Check return type.
– Verify the type match of two operands when assigning.
– Check the argument number when calling function.
– Check if conditional of “If” or “While” has a value.
– Check other things by referring to C-Minus syntax.
– Note: C-minus Type  void, int, int[]
*/

static void checkNode(TreeNode* t)
{
    // switch (t->nodekind)
    // {
    //     case ExpK:
    //         switch (t->kind.exp)
    //         {
    //             case AssignmentK:
    //             {
    //                 ExpType lhs_type = t->child[0]->type;
    //                 ExpType rhs_type = t->child[1]->type;

    //                 if (lhs_type != Integer || rhs_type != Integer)
    //                 {
    //                     typeError(
    //                         t, "assignment can only be done between
    //                         integers.");
    //                     break;
    //                 }
    //                 if (t->child[0]->isarray != t->child[1]->isarray)
    //                 {
    //                     typeError(t,
    //                               "assignment between int array and int is
    //                               not " "possible.");
    //                     break;
    //                 }
    //                 t->type = Integer;
    //                 t->isarray = t->child[0]->isarray;
    //                 break;
    //             }
    //             case OperatorK:
    //             {
    //                 ExpType lhs_type = t->child[0]->type;
    //                 ExpType rhs_type = t->child[1]->type;

    //                 if (lhs_type != Integer || rhs_type != Integer)
    //                 {
    //                     typeError(t, "invalid operand type");
    //                 }
    //                 if (t->child[0]->isarray || t->child[1]->isarray)
    //                 {
    //                     typeError(
    //                         t,
    //                         "operations between array names are not
    //                         possible.");
    //                     break;
    //                 }
    //                 t->type = Integer;
    //                 t->isarray = FALSE;
    //                 break;
    //             }
    //             case CallK:
    //             {
    //                 BucketList bucket =
    //                     st_lookup(scope_stack_top().scope, t->attr.name);
    //                 if (!bucket)
    //                 {
    //                     break;
    //                 }

    //                 // count arguments
    //                 int count = 0;
    //                 TreeNode* node = t->child[0];
    //                 while (node)
    //                 {
    //                     ++count;
    //                     node = node->sibling;
    //                 }

    //                 // TODO:
    //                 break;
    //             }
    //             case ConstantK:
    //                 t->type = Integer;
    //                 t->isarray = FALSE;
    //                 break;
    //             case VarK:
    //                 // TODO:
    //                 break;
    //             case TypeK:
    //                 t->type = Integer;
    //                 break;
    //             default:
    //                 break;
    //         }
    //         break;
    //     case StmtK:
    //         switch (t->kind.stmt)
    //         {
    //             case CompoundK:
    //             case SelectionK:
    //             case IterationK:
    //             case ReturnK:
    //             default:
    //                 break;
    //         }
    //         break;
    //     case DeclarationK:
    //         switch (t->kind.declaration)
    //         {
    //             case FuncK:
    //             case VarDeclarationK:
    //             case ParameterK:
    //             case VoidParameterK:
    //             default:
    //                 break;
    //         }
    //     default:
    //         break;
    // }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode* syntaxTree)
{
    traverse(syntaxTree, nullProc, checkNode);
}

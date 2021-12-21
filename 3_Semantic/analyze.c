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

static void argCountError(TreeNode* t, char* funcName, int paramCount, int argCount)
{
    fprintf(listing, "function call error at line %d: The %s function has %d parameters, but only %d entered.\n", t->lineno, funcName, paramCount, argCount);
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
// static void nullProc(TreeNode* t)
// {
//     if (t == NULL)
//         return;
//     else
//         return;
// }

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
        BucketList output = st_insert(scope, "output", Void, FALSE, FuncSymbol, 0, location++);
        TreeNode param;
        param.isarray = FALSE;
        param.attr.name = "";
        param.type = Integer;
        addFuncArg(output, &param);
    }

    // input
    {
        st_insert(scope, "input", Integer, FALSE, FuncSymbol, 0, location++);
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

static BucketList current_function;
/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode(TreeNode* t)
{
    static int is_func_compound = FALSE;
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
                        // t->type = Invalid;
                        // undeclaredError(t);
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
                        ScopeList newScope =
                            create_ScopeList(pair->scope, "compound");
                        scope_stack_push(newScope, 0);
                        t->scope = newScope;
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
                    ScopeList newScope =
                        create_ScopeList(pair->scope, t->attr.name);
                    scope_stack_push(newScope, 0);
                    t->scope = newScope;
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

static void afterInsertNode(TreeNode* t)
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

static void beforeCheckNode(TreeNode* t)
{
    switch (t->nodekind)
    {
        case ExpK:
            switch (t->kind.exp)
            {
                case AssignmentK:
                case OperatorK:
                case ConstantK:
                case CallK:
                case VarK:
                default:
                    break;
            }
            break;
        case StmtK:
            switch (t->kind.stmt)
            {
                case CompoundK:
                    if (t->scope)
                    {
                        scope_stack_push(t->scope, 0);
                    }
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
                    scope_stack_push(t->scope, 0);
                    current_function = st_lookup(t->scope, t->attr.name);
                    break;
                case VarDeclarationK:
                case ParameterK:
                case VoidParameterK:
                default:
                    break;
            }
        default:
            break;
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
    ScopeStackPair* pair = scope_stack_top();
    switch (t->nodekind)
    {
        case ExpK:
            switch (t->kind.exp)
            {
                case AssignmentK:
                {
                    ExpType lhs_type = t->child[0]->type;
                    ExpType rhs_type = t->child[1]->type;

                    if (lhs_type == Invalid || rhs_type == Invalid)
                    {
                        break;
                    }

                    if (lhs_type != Integer || rhs_type != Integer)
                    {
                        typeError(
                            t, "assignment can only be done between integers.");
                        break;
                    }
                    if (t->child[0]->isarray != t->child[1]->isarray)
                    {
                        typeError(t,
                                  "assignment between int array and int is not possible.");
                        break;
                    }
                    t->type = Integer;
                    t->isarray = t->child[0]->isarray;
                    break;
                }
                case OperatorK:
                {
                    ExpType lhs_type = t->child[0]->type;
                    ExpType rhs_type = t->child[1]->type;

                    if (lhs_type == Invalid || rhs_type == Invalid)
                    {
                        break;
                    }

                    if (lhs_type != Integer || rhs_type != Integer)
                    {
                        typeError(t, "invalid operand type");
                    }
                    if (t->child[0]->isarray || t->child[1]->isarray)
                    {
                        typeError(
                            t,
                            "operations between array names are not possible.");
                        break;
                    }
                    t->type = Integer;
                    t->isarray = FALSE;
                    break;
                }
                case CallK:
                {
                    BucketList bucket =
                        st_lookup(pair->scope, t->attr.name);
                    if (!bucket)
                    {
                        t->type = Invalid;
                        t->isarray = FALSE;
                        undeclaredError(t);
                        break;
                    }

                    // count arguments
                    int count = 0;
                    TreeNode* node = t->child[0];
                    while (node)
                    {
                        ++count;
                        node = node->sibling;
                    }

                    if (count != bucket->functionInfo.args_count)
                    {
                        argCountError(t, bucket->name, bucket->functionInfo.args_count, count);
                        break;
                    }

                    FunctionArgsList arg = bucket->functionInfo.args;
                    node = t->child[0];
                    for (int j = 0; j < count; ++j)
                    {
                        if (arg->type != node->type)
                        {
                            char buf[101];
                            snprintf(buf, 100, "The type of %dth argument of '%s' is different", j + 1, bucket->name);
                            typeError(t, buf);
                        }
                    }

                    // TODO:
                    t->type = bucket->type;
                    t->isarray = bucket->isarray;
                    break;
                }
                case ConstantK:
                    t->type = Integer;
                    t->isarray = FALSE;
                    break;
                case VarK:
                {
                    BucketList bucket = st_lookup(pair->scope, t->attr.name);
                    if (!bucket)
                    {
                        t->type = Invalid;
                        t->isarray = FALSE;
                        undeclaredError(t);
                        break;
                    }
                    t->type = bucket->type;
                    if (!bucket->isarray && t->child[0])
                    {
                        typeError(t, "Cannot use the [] operator on non-array variables.");
                        break;
                    }
                    if (t->child[0] && (t->child[0]->type != Integer || t->child[0]->isarray))
                    {
                        typeError(t, "The index of the array must be integer.");
                        break;
                    }
                    t->isarray = bucket->isarray && !t->child[0];
                    break;
                }
                default:
                    break;
            }
            break;
        case StmtK:
            switch (t->kind.stmt)
            {
                case CompoundK:
                    scope_stack_pop();
                    break;
                case SelectionK:
                    if (!t->child[0])
                    {
                        typeError(t->child[0], "The conditional statement of if must not be empty");
                    }
                    else if (t->child[0]->type == Invalid)
                    {
                        // do nothing
                    }
                    else if (t->child[0]->type != Integer || t->child[0]->isarray)
                    {
                        typeError(t->child[0], "The type of if condition can only be integer.");
                    }
                    break;
                case IterationK:
                    if (!t->child[0])
                    {
                        typeError(t, "The conditional statement of loop must not be empty");
                    }
                    else if (t->child[0]->type == Invalid)
                    {
                        // do nothing
                    }
                    else if (t->child[0]->type != Integer || t->child[0]->isarray)
                    {
                        typeError(t->child[0], "The type of loop condition can only be integer.");
                    }
                    break;
                case ReturnK:
                {
                    if (current_function->type == Void && t->child[0])
                    {
                        typeError(t->child[0], "Function of type 'void' cannot return a value.");
                    }
                    else if (current_function->type == Integer && !current_function->isarray)
                    {
                        if (!t->child[0])
                        {
                            typeError(t, "The return statement of int type function must contain a value.");
                        }
                        else if (t->child[0]->type == Invalid)
                        {
                            // do nothing
                        }
                        else if (t->child[0]->type != current_function->type || t->child[0]->isarray != current_function->isarray)
                        {
                            typeError(t->child[0], "The type of function and the type of the return value must always be the same");
                        }
                        else
                        {
                            // no problem
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case DeclarationK:
            switch (t->kind.declaration)
            {
                case VarDeclarationK:
                case ParameterK:
                    if (t->type != Integer)
                    {
                        typeError(t, "Variable type must be integer or integer array");
                    }
                    break;
                case FuncK:
                    t->type = current_function->type;
                    t->isarray = current_function->isarray;
                    break;
                case VoidParameterK:
                default:
                    break;
            }
        default:
            break;
    }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode* syntaxTree)
{
    traverse(syntaxTree, beforeCheckNode, checkNode);
}

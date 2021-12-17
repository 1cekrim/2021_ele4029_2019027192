/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedNum;     /* ID [ NUM ] */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex
int yyerror(char * message);

%}

%token IF ELSE WHILE RETURN INT VOID
%token ID NUM 
%token ASSIGN EQ NE LT LE GT GE PLUS MINUS TIMES OVER LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR 
%nonassoc IF_REDUCE
%nonassoc ELSE

%% /* Grammar for C- */

program             : declaration_list
                      {
                        savedTree = $1;
                      } 
                    ;

declaration_list    : declaration_list declaration
                      {
                        YYSTYPE t = $1;
                        if (t != NULL)
                        {
                          while (t->sibling != NULL)
                          {
                            t = t->sibling;
                          }
                          t->sibling = $2;
                          $$ = $1;
                        }
                        else
                        {
                          $$ = $2;
                        }
                      }
                    | declaration
                      {
                        $$ = $1; 
                      }
                    ;

declaration         : var_declaration
                      {
                        $$ = $1; 
                      }
                    | fun_declaration
                      {
                        $$ = $1; 
                      }
                    ;

var_declaration     : type_specifier saveName SEMI
                      {
                        $$ = newDeclarationNode(VarDeclarationK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                        $$->type = $1->type;
                      }
                    | type_specifier saveName LBRACE saveNum RBRACE SEMI
                      {
                        $$ = newDeclarationNode(VarDeclarationK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                        $$->type = $1->type;
                        $$->child[0] = newExpNode(ConstantK);
                        $$->child[0]->attr.val = savedNum;
                        $$->isarray = TRUE;
                      }
                    ;

saveName            : ID
                      {
                        savedName = copyString(tokenString);
                        savedLineNo = lineno;
                      }
                    ;

saveNum             : NUM
                      {
                        savedNum = atoi(tokenString);
                        savedLineNo = lineno;
                      }
                    ;

type_specifier      : INT
                      {
                        $$ = newExpNode(TypeK);
                        $$->type = Integer;
                      }
                    | VOID
                      {
                        $$ = newExpNode(TypeK);
                        $$->type = Void;
                      }
                    ;

fun_declaration     : type_specifier saveName
                      {
                        $$ = newDeclarationNode(FuncK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                        $$->type = $1->type;
                      }
                      LPAREN params RPAREN compound_stmt
                      {
                        $$ = $3;
                        $$->child[0] = $5;
                        $$->child[1] = $7;
                      }
                    ;

params              : param_list
                      {
                        $$ = $1;
                      }
                    | VOID
                      {
                        $$ = newDeclarationNode(VoidParameterK);
                      }
                    ;

param_list          : param_list COMMA param
                      {
                        YYSTYPE t = $1;
                        if (t != NULL)
                        {
                          while (t->sibling != NULL)
                          {
                            t = t->sibling;
                          }
                          t->sibling = $3;
                          $$ = $1;
                        }
                        else
                        {
                          $$ = $3;
                        }
                      }
                    | param
                      {
                        $$ = $1; 
                      }
                    ;
                    
param               : type_specifier saveName
                      {
                        $$ = newDeclarationNode(ParameterK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                        $$->type = $1->type;
                      }
                    | type_specifier saveName
                      {
                        $$ = newDeclarationNode(ParameterK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                        $$->type = $1->type;
                      }
                      LBRACE RBRACE
                      {
                        $$ = $3;
                        $$->isarray = TRUE;
                      }
                    ;

compound_stmt       : LCURLY local_declarations statement_list RCURLY
                      {
                        $$ = newStmtNode(CompoundK);
                        $$->child[0] = $2;
                        $$->child[1] = $3;
                      }
                    ;

local_declarations  : local_declarations var_declaration
                      {
                        YYSTYPE t = $1;
                        if (t != NULL)
                        {
                          while (t->sibling != NULL)
                          {
                            t = t->sibling;
                          }
                          t->sibling = $2;
                          $$ = $1;
                        }
                        else
                        {
                          $$ = $2;
                        }
                      }
                    | /* empty */
                      {
                        $$ = NULL;
                      }
                    ;

statement_list      : statement_list statement
                      {
                        YYSTYPE t = $1;
                        if (t != NULL)
                        {
                          while (t->sibling != NULL)
                          {
                            t = t->sibling;
                          }
                          t->sibling = $2;
                          $$ = $1;
                        }
                        else
                        {
                          $$ = $2;
                        }
                      }
                    | /* empty */
                      {
                        $$ = NULL;
                      }
                    ;

statement           : expression_stmt
                      {
                        $$ = $1;
                      }
                    | compound_stmt
                      {
                        $$ = $1;
                      }
                    | selection_stmt
                      {
                        $$ = $1;
                      }
                    | iteration_stmt
                      {
                        $$ = $1;
                      }
                    | return_stmt
                      {
                        $$ = $1;
                      }
                    ;

expression_stmt     : expression SEMI
                      {
                        $$ = $1;
                      }
                    | SEMI
                      {
                        $$ = NULL;
                      }
                    ;

selection_stmt      : IF LPAREN expression RPAREN statement %prec IF_REDUCE
                      {
                        $$ = newStmtNode(SelectionK);
                        $$->child[0] = $3;
                        $$->child[1] = $5;
                      }
                    | IF LPAREN expression RPAREN statement ELSE statement
                      {
                        $$ = newStmtNode(SelectionK);
                        $$->child[0] = $3;
                        $$->child[1] = $5;
                        $$->child[2] = $7;
                      }
                    ;

iteration_stmt      : WHILE LPAREN expression RPAREN statement
                      {
                        $$ = newStmtNode(IterationK);
                        $$->child[0] = $3;
                        $$->child[1] = $5;
                      }
                    ;

return_stmt         : RETURN SEMI
                      {
                        $$ = newStmtNode(ReturnK);
                      }
                    | RETURN expression SEMI
                      {
                        $$ = newStmtNode(ReturnK);
                        $$->child[0] = $2;
                      }
                    ;

expression          : var ASSIGN expression
                      {
                        $$ = newExpNode(AssignmentK);
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                      }
                    | simple_expression
                      {
                        $$ = $1;
                      }
                    ;

var                 : saveName
                      {
                        $$ = newExpNode(VarK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                      }
                    | saveName
                      {
                        $$ = newExpNode(VarK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                      }
                      LBRACE expression RBRACE
                      {
                        $$ = $2;
                        $$->child[0] = $4;
                      }
                    ;

simple_expression   : additive_expression relop additive_expression
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = $2->attr.op;
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                      }
                    | additive_expression
                      {
                        $$ = $1;
                      }
                    ;

relop               : LE
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = LE;
                      }
                    | LT
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = LT;
                      }
                    | GT
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = GT;
                      }
                    | GE
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = GE;
                      }
                    | EQ
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = EQ;
                      }
                    | NE
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = NE;
                      }
                    ;

additive_expression : additive_expression addop term
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = $2->attr.op;
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                      }
                    | term
                      {
                        $$ = $1;
                      }
                    ;

addop               : PLUS
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = PLUS;
                      }
                    | MINUS
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = MINUS;
                      }
                    ;

term                : term mulop factor
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = $2->attr.op;
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                      }
                    | factor
                      {
                        $$ = $1;
                      }
                    ;

mulop               : TIMES
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = TIMES;
                      }
                    | OVER
                      {
                        $$ = newExpNode(OperatorK);
                        $$->attr.op = OVER;
                      }
                    ;

factor              : LPAREN expression RPAREN
                      {
                        $$ = $2;
                      }
                    | var
                      {
                        $$ = $1;
                      }
                    | call
                      {
                        $$ = $1; 
                      }
                    | saveNum
                      {
                        $$ = newExpNode(ConstantK);
                        $$->attr.val = savedNum;
                        $$->lineno = savedLineNo;
                      }
                    ;

call                : saveName
                      {
                        $$ = newExpNode(CallK);
                        $$->attr.name = savedName;
                        $$->lineno = savedLineNo;
                      }
                      LPAREN args RPAREN
                      {
                        $$ = $2;
                        $$->child[0] = $4;
                      }
                    ;

args                : arg_list
                      {
                        $$ = $1;
                      }
                    | /* empty */
                      {
                        $$ = NULL;
                      }
                    ;

arg_list            : arg_list COMMA expression
                      {
                        YYSTYPE t = $1;
                        if (t != NULL)
                        {
                          while (t->sibling != NULL)
                          {
                            t = t->sibling;
                          }
                          t->sibling = $3;
                          $$ = $1;
                        }
                        else
                        {
                          $$ = $3;
                        }
                      }
                    | expression
                      {
                        $$ = $1;
                      }
                    ;

%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}


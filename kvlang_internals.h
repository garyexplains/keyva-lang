/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (C) 2024 Gary Sims
 *
 */

#ifndef KVLANGINTERNALS_H
#define KVLANGINTERNALS_H

#define MAX_LINE_LENGTH 1024
#define MAX_TOKEN_LENGTH 256
#define MAX_TOKENS_PER_LINE 100
#define MAX_TOKENS MAX_TOKENS_PER_LINE
#define MAX_FUNC_PARAMS 10

// Token definitions
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_DELIMITER,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN
} TokenType;

typedef enum {
    EVAL_ARITHMETIC, // For arithmetic expressions
    EVAL_PRINT       // For the print function
} EvalContext;

typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LENGTH];
} Token;

typedef enum {
    AST_PRINT = 0,              // 0
    AST_LITERAL,                // 1
    AST_IDENTIFIER,             // 2
    AST_ASSIGNMENT,             // 3
    AST_ARRAY_ACCESS,           // 4
    AST_BINARY_OP,              // 5
    AST_IF_STATEMENT,           // 6
    AST_BLOCK,                  // 7
    AST_FOR_STATEMENT,          // 8
    AST_WHILE_STATEMENT,        // 9
    AST_FUNCTION_DEFINITION,    // 10
    AST_FUNCTION_CALL,          // 11
    AST_RETURN_STATEMENT,       // 12
    // ... other AST node types ...
} ASTNodeType;

typedef enum {
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_LESS_THAN,
    OP_GREATER_THAN,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS_EQUAL,
    OP_GREATER_EQUAL,
    // ... other operators ...
} OperatorType;

struct ASTNode;

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    struct ASTNode *parameters;  // Linked list of parameter identifiers
    struct ASTNode *body;        // Block of statements
} FunctionDefinition;

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    struct ASTNode *arguments;    // Linked list of expressions for arguments
} FunctionCall;

typedef struct {
    struct ASTNode *expression;   // Expression to return
} ReturnStatement;

typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *nextblock;
    union {
        // For binary operators
        OperatorType operator;
        // For literals and identifiers
        char string_value[MAX_TOKEN_LENGTH];
        char identifier[MAX_TOKEN_LENGTH];
        // For function definitions
        FunctionDefinition func_def;
        // For function calls
        FunctionCall func_call;
        // For return statements
        ReturnStatement ret_stmt;
        // For if statements
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_stmt;
        // For for statements
        struct {
            char loop_var[MAX_TOKEN_LENGTH];  // Name of the loop variable
            struct ASTNode *expression;       // Expression that should yield an array
            struct ASTNode *body;             // Block of statements
        } for_stmt;
        struct {
            struct ASTNode *condition; // The condition expression
            struct ASTNode *body;      // The block of statements to execute
        } while_stmt;
        
    } data;
} ASTNode;

typedef struct {
    char key[MAX_TOKEN_LENGTH];
    char value[MAX_TOKEN_LENGTH];
} KeyValuePair;

typedef struct {
    KeyValuePair *pairs;
    int size;
    int capacity;
} AssocArray;

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    AssocArray array;
} Variable;

typedef enum {
    RESULT_STRING,
    RESULT_NUMBER,
    RESULT_ASSOC_ARRAY
} ResultType;

typedef struct {
    ResultType type;
    union {
        char string_value[MAX_TOKEN_LENGTH];
        double number_value;
        AssocArray *array_value;
    };
} EvalResult;


typedef struct {
    char name[MAX_TOKEN_LENGTH];
    struct ASTNode *parameters;
    struct ASTNode *body;
} FunctionEntry;

typedef struct {
    int has_return;
    ResultType type;
    union {
        char string_value[MAX_TOKEN_LENGTH];
        double number_value;
        AssocArray *array_value;
    };
    // char return_value[MAX_TOKEN_LENGTH]; // store number or string
    // int return_is_number;
    // If you have a Value struct, store it instead
} FunctionReturn;


// Function declarations
void tokenize_line(const char *line, Token tokens[], int *token_count);
void duplicate_assoc_array(AssocArray *dup, AssocArray *array);
void parse_and_execute(Token tokens[], int token_count);
ASTNode* parse_print_statement(Token tokens[], int *pos, int token_count);
void execute_ast(ASTNode *node);
ASTNode* parse_comparison(Token tokens[], int *pos, int token_count);
int evaluate_expression(ASTNode *node, EvalResult *result, EvalContext context);
void evaluate_and_print(ASTNode *node);
void free_ast(ASTNode *node);
ASTNode* parse_assignment_statement(Token tokens[], int *pos, int token_count);
ASTNode* parse_term(Token tokens[], int *pos, int token_count);
ASTNode* parse_factor(Token tokens[], int *pos, int token_count);
ASTNode* parse_expression(Token tokens[], int *pos, int token_count);
ASTNode* parse_additive(Token tokens[], int *pos, int token_count);
void execute_assignment(ASTNode *node);
Variable* get_variable(const char *name);
char* get_variable_value(const char *name);
void set_variable_value(const char *name, const char *key, const char *value);
void clear_variable_assoc_array(const char *name);
ASTNode* parse_if_statement(Token tokens[], int *pos, int token_count);
ASTNode* parse_statement(Token tokens[], int *pos, int token_count);
ASTNode* parse_block(Token tokens[], int *pos, int token_count);
ASTNode* parse_for_statement(Token tokens[], int *pos, int token_count);
ASTNode* parse_while_statement(Token tokens[], int *pos, int token_count);
ASTNode* parse_function_call(Token tokens[], int *pos, int token_count);
void execute_block(ASTNode *node);
FunctionReturn execute_ast_with_return(ASTNode *node);
FunctionReturn execute_block_with_return(ASTNode *node);
FunctionReturn execute_function_call(ASTNode *call_node);
ASTNode* parse_return_statement(Token tokens[], int *pos, int token_count);
ASTNode* parse_function_definition(Token tokens[], int *pos, int token_count);

#endif /* KVLANGINTERNALS_H */
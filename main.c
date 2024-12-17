/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (C) 2024 Gary Sims
 *
 */

/*
 * Build instructions:
 *
 * gcc -O3 -o keyva main.c kvstdlib.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NDEBUG 1
//#define DEBUG 1
#include "debug_print.h"

#include "kvlang_internals.h"

#include "kvstdlib.h"

#define MAX_FUNCTIONS 100
FunctionEntry functions[MAX_FUNCTIONS];
int function_count = 0;

#define MAX_VARIABLES 100
Variable variables[MAX_VARIABLES];
int variable_count = 0;

// Keyword, operator, and delimiter definitions
const char *keywords[] = {
    "def", "return", "end", "if", "else", "print", "for", "in", "while", NULL
};

const char *operators[] = {
    "+", "-", "*", "/", "=", "<", ">", "<=", ">=", "==", "!=", NULL
};

const char *delimiters = "(),[]";

int is_keyword(const char *str) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0)
            return 1;
    }
    return 0;
}

int is_operator_char(char c) {
    return strchr("+-*/=<>!", c) != NULL;
}

int is_operator(const char *str) {
    for (int i = 0; operators[i] != NULL; i++) {
        if (strcmp(str, operators[i]) == 0)
            return 1;
    }
    return 0;
}

int is_delimiter_char(char c) {
    return strchr(delimiters, c) != NULL;
}

void tokenize_line(const char *line, Token tokens[], int *token_count) {
    int pos = 0;
    int length = strlen(line);
    *token_count = 0;

    while (pos < length) {
        char c = line[pos];

        // Skip whitespace
        if (isspace(c)) {
            pos++;
            continue;
        }

        // Comments
        if (c == '#') {
            // Rest of the line is a comment
            Token token;
            token.type = TOKEN_COMMENT;
            strncpy(token.value, line + pos, MAX_TOKEN_LENGTH - 1);
            token.value[MAX_TOKEN_LENGTH - 1] = '\0';
            tokens[(*token_count)++] = token;
            break; // Ignore rest of the line
        }

        // String literals
        if (c == '"' || c == '\'') {
            char quote = c;
            int start = ++pos;
            while (pos < length && line[pos] != quote) {
                pos++;
            }
            if (pos >= length) {
                printf("Error: Unterminated string literal\n");
                return;
            }
            Token token;
            token.type = TOKEN_STRING;
            int str_length = pos - start;
            strncpy(token.value, line + start, str_length);
            token.value[str_length] = '\0';
            tokens[(*token_count)++] = token;
            pos++; // Skip closing quote
            continue;
        }

        // Numbers
        if (isdigit(c)) {
            int start = pos;
            while (pos < length && isdigit(line[pos])) {
                pos++;
            }
            Token token;
            token.type = TOKEN_NUMBER;
            int num_length = pos - start;
            strncpy(token.value, line + start, num_length);
            token.value[num_length] = '\0';
            tokens[(*token_count)++] = token;
            continue;
        }

        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            int start = pos;
            while (pos < length && (isalnum(line[pos]) || line[pos] == '_')) {
                pos++;
            }
            int id_length = pos - start;
            char id[MAX_TOKEN_LENGTH];
            strncpy(id, line + start, id_length);
            id[id_length] = '\0';

            Token token;
            if (is_keyword(id)) {
                token.type = TOKEN_KEYWORD;
            } else {
                token.type = TOKEN_IDENTIFIER;
            }
            strcpy(token.value, id);
            tokens[(*token_count)++] = token;
            continue;
        }

        // Operators
        if (is_operator_char(c)) {
            int start = pos;
            while (pos < length && is_operator_char(line[pos])) {
                pos++;
            }
            int op_length = pos - start;
            char op[MAX_TOKEN_LENGTH];
            strncpy(op, line + start, op_length);
            op[op_length] = '\0';

            // Check if it's a valid operator
            if (is_operator(op)) {
                Token token;
                token.type = TOKEN_OPERATOR;
                strcpy(token.value, op);
                tokens[(*token_count)++] = token;
            } else {
                printf("Error: Unknown operator '%s'\n", op);
            }
            continue;
        }

        // Delimiters
        if (is_delimiter_char(c)) {
            Token token;
            token.type = TOKEN_DELIMITER;
            token.value[0] = c;
            token.value[1] = '\0';
            tokens[(*token_count)++] = token;
            pos++;
            continue;
        }

        // Unknown character
        printf("Error: Unknown character '%c'\n", c);
        pos++;
    }
}

void init_assoc_array(AssocArray *array) {
    array->size = 0;
    array->capacity = 4; // Initial capacity
    array->pairs = (KeyValuePair *)malloc(sizeof(KeyValuePair) * array->capacity);
}

void free_assoc_array(AssocArray *array) {
    free(array->pairs);
    array->pairs = NULL;
    array->size = 0;
    array->capacity = 0;
}

void duplicate_assoc_array(AssocArray *dup, AssocArray *array) {
    dup->capacity = array->capacity;
    dup->size = array->size;
    dup->pairs = (KeyValuePair *)malloc(sizeof(KeyValuePair) * array->capacity);
    memcpy(dup->pairs, array->pairs, sizeof(KeyValuePair) * array->capacity);
}

void set_assoc_array_value(AssocArray *array, const char *key, const char *value) {
    // Check if key exists
    for (int i = 0; i < array->size; i++) {
        if (strcmp(array->pairs[i].key, key) == 0) {
            strcpy(array->pairs[i].value, value);
            return;
        }
    }
    // Add new key-value pair
    if (array->size == array->capacity) {
        array->capacity *= 2;
        array->pairs = (KeyValuePair *)realloc(array->pairs, sizeof(KeyValuePair) * array->capacity);
    }
    strcpy(array->pairs[array->size].key, key);
    strcpy(array->pairs[array->size].value, value);
    array->size++;
}

char* get_assoc_array_value(AssocArray *array, const char *key) {
    for (int i = 0; i < array->size; i++) {
        if (strcmp(array->pairs[i].key, key) == 0) {
            return array->pairs[i].value;
        }
    }
    return NULL; // Key not found
}

char* get_first_assoc_array_value(AssocArray *array) {
    return array->pairs[0].value;
}

ASTNode* parse_array_access(Token tokens[], int *pos, int token_count) {
    // tokens[*pos] should be an identifier
    if (tokens[*pos].type != TOKEN_IDENTIFIER) {
        printf("Error: Expected identifier for array access\n");
        return NULL;
    }
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ARRAY_ACCESS;
    node->left = NULL;
    node->right = NULL;
    node->nextblock = NULL;
    // DEBUG_PRINT("Add AST_ARRAY_ACCESS Node of %d", node->type);
    strcpy(node->data.identifier, tokens[*pos].value);
    (*pos)++;

    // Expect '['
    if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == '[') {
        (*pos)++;
        // Parse the key expression inside brackets
        ASTNode *key_expr = parse_expression(tokens, pos, token_count);
        if (key_expr == NULL) {
            printf("Error: Expected expression inside '[]'\n");
            free_ast(node);
            return NULL;
        }
        node->left = key_expr;
        node->right = NULL;
        node->nextblock = NULL;

        // Expect ']'
        if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ']') {
            (*pos)++;
            return node;
        } else {
            printf("Error: Expected ']' after array index\n");
            free_ast(node);
            return NULL;
        }
    } else {
        printf("Error: Expected '[' after identifier for array access\n");
        free_ast(node);
        return NULL;
    }
}

ASTNode* parse_assignment_statement(Token tokens[], int *pos, int token_count) {
    // Expect an identifier
    if (tokens[*pos].type == TOKEN_IDENTIFIER) {
        ASTNode *target = NULL;

        // Check for array access (e.g., a["lemon"])
        if ((*pos + 1) < token_count && tokens[*pos + 1].type == TOKEN_DELIMITER && tokens[*pos + 1].value[0] == '[') {
            target = parse_array_access(tokens, pos, token_count);
        } else {
            // Simple identifier
            target = (ASTNode*)malloc(sizeof(ASTNode));
            target->type = AST_IDENTIFIER;
            // DEBUG_PRINT("Add AST_IDENTIFIER Node of %d", target->type);
            strcpy(target->data.identifier, tokens[*pos].value);
            target->left = target->right = NULL;
            target->nextblock = NULL;
            (*pos)++;
        }

        // Expect '=' operator
        if (*pos < token_count && tokens[*pos].type == TOKEN_OPERATOR && strcmp(tokens[*pos].value, "=") == 0) {
            (*pos)++;
            // Parse the expression on the right-hand side
            ASTNode *expr = parse_expression(tokens, pos, token_count);
            if (expr == NULL) {
                printf("Error: Expected expression after '='\n");
                free_ast(target);
                return NULL;
            }
            // Create assignment node
            ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_ASSIGNMENT;
            // DEBUG_PRINT("Add AST_ASSIGNMENT Node of %d", node->type);
            node->left = target;
            node->right = expr;
            node->nextblock = NULL;
            return node;
        } else {
            printf("Error: Expected '=' after identifier\n");
            free_ast(target);
            return NULL;
        }
    }
    return NULL;
}


void parse_and_execute(Token tokens[], int token_count) {
    int pos = 0;

    while (pos < token_count) {
        ASTNode *node = parse_statement(tokens, &pos, token_count);
        if (node != NULL) {
            execute_ast(node);
            free_ast(node);
        } else {
            // Skip the rest of the line on error
            break;
        }
    }
}

ASTNode* parse_phrase(Token tokens[], int *pos, int token_count) {
    if (*pos >= token_count) {
        printf("Error: Unexpected end of input in term\n");
        return NULL;
    }

    Token token = tokens[*pos];

    if (token.type == TOKEN_NUMBER || token.type == TOKEN_STRING) {
        // Literal
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        if (node == NULL) {
            printf("Error: Memory allocation failed\n");
            return NULL;
        }
        node->type = AST_LITERAL;
        // DEBUG_PRINT("Add AST_LITERAL Node of %d", node->type);
        node->left = NULL;
        node->right = NULL;
        node->nextblock = NULL;
        strcpy(node->data.string_value, token.value);
        (*pos)++;
        return node;
    } else if (token.type == TOKEN_IDENTIFIER) {
        // Check for array access
        if ((*pos + 1) < token_count &&
            tokens[*pos + 1].type == TOKEN_DELIMITER &&
            tokens[*pos + 1].value[0] == '[') {
            return parse_array_access(tokens, pos, token_count);
        } else if ((*pos + 1) < token_count &&
            tokens[*pos + 1].type == TOKEN_DELIMITER &&
            tokens[*pos + 1].value[0] == '(') {
            // Function call 
            char ident[MAX_TOKEN_LENGTH];
            strcpy(ident, tokens[*pos].value);
            (*pos)++;

            // Parse arguments
            (*pos)++;
            ASTNode *arg_list = NULL;
            ASTNode **current = &arg_list;
            while (*pos < token_count && !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
                ASTNode *arg = parse_expression(tokens, pos, token_count);
                if (arg == NULL) {
                    printf("Error: Expected expression in function call arguments\n");
                    free_ast(arg_list);
                    return NULL;
                }
                *current = arg;
                current = &arg->right;

                // Check for comma
                if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ',') {
                    (*pos)++;
                }
            }

            // Expect ')'
            if (*pos >= token_count || !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
                printf("Error: Expected ')' after function call arguments\n");
                //free_ast(arg_list);
                return NULL;
            }
            (*pos)++;

            // Create function call node
            ASTNode *call_node = (ASTNode*)malloc(sizeof(ASTNode));
            call_node->type = AST_FUNCTION_CALL;
            // DEBUG_PRINT("Add AST_FUNCTION_CALL Node of %d", call_node->type);
            call_node->left = call_node->right = NULL;
            call_node->nextblock = NULL;
            strcpy(call_node->data.func_call.name, ident);
            call_node->data.func_call.arguments = arg_list;
            //call_node->data.func_call.arguments = NULL;
            return call_node;               
        } else {
            // Simple identifier
            ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
            if (node == NULL) {
                printf("Error: Memory allocation failed\n");
                return NULL;
            }
            node->type = AST_IDENTIFIER;
            // DEBUG_PRINT("Add AST_IDENTIFIER Node of %d", node->type);
            node->left = NULL;
            node->right = NULL;
            node->nextblock = NULL;
            strcpy(node->data.identifier, token.value);
            (*pos)++;
            return node;
        }
    } else if (token.type == TOKEN_DELIMITER && token.value[0] == '(') {
        // Parenthesized expression
        (*pos)++;
        ASTNode *node = parse_expression(tokens, pos, token_count);
        if (node == NULL) {
            return NULL;
        }
        if (*pos >= token_count ||
            tokens[*pos].type != TOKEN_DELIMITER ||
            tokens[*pos].value[0] != ')') {
            printf("Error: Expected ')' after expression\n");
            free_ast(node);
            return NULL;
        }
        (*pos)++;
        return node;
    }

    printf("Error: Unexpected token '%s' in term\n", token.value);
    return NULL;
}

// ASTNode* parse_comparison(Token tokens[], int *pos, int token_count) {
//     ASTNode *left = parse_phrase(tokens, pos, token_count);
//     if (left == NULL) {
//         return NULL;
//     }

//     while (*pos < token_count) {
//         Token token = tokens[*pos];

//         OperatorType op_type;
//         int is_comparison = 0;

//         if (token.type == TOKEN_OPERATOR) {
//             if (strcmp(token.value, "<") == 0) {
//                 op_type = OP_LESS_THAN;
//                 is_comparison = 1;
//             } else if (strcmp(token.value, ">") == 0) {
//                 op_type = OP_GREATER_THAN;
//                 is_comparison = 1;
//             } else if (strcmp(token.value, "==") == 0) {
//                 op_type = OP_EQUAL;
//                 is_comparison = 1;
//             } else if (strcmp(token.value, "!=") == 0) {
//                 op_type = OP_NOT_EQUAL;
//                 is_comparison = 1;
//             } else if (strcmp(token.value, "<=") == 0) {
//                 op_type = OP_LESS_EQUAL;
//                 is_comparison = 1;
//             } else if (strcmp(token.value, ">=") == 0) {
//                 op_type = OP_GREATER_EQUAL;
//                 is_comparison = 1;
//             }
//         }

//         if (!is_comparison) {
//             break; // No more comparison operators
//         }

//         (*pos)++; // Consume operator
//         ASTNode *right = parse_phrase(tokens, pos, token_count);
//         if (right == NULL) {
//             free_ast(left);
//             return NULL;
//         }

//         // Create binary operation node
//         ASTNode *op_node = (ASTNode*)malloc(sizeof(ASTNode));
//         op_node->type = AST_BINARY_OP;
//         // DEBUG_PRINT("Add AST_BINARY_OP Node of %d", op_node->type);
//         op_node->data.operator = op_type;
//         op_node->left = left;
//         op_node->right = right;
//         op_node->nextblock = NULL;

//         left = op_node; // Update left node for the next iteration
//     }

//     return left;
// }

// ASTNode* parse_expression(Token tokens[], int *pos, int token_count) {
//     ASTNode *left = parse_comparison(tokens, pos, token_count);
//     if (left == NULL) {
//         return NULL;
//     }

//     while (*pos < token_count) {
//         Token token = tokens[*pos];

//         if (token.type == TOKEN_OPERATOR && (strcmp(token.value, "+") == 0 || strcmp(token.value, "-") == 0)) {
//             OperatorType op_type = (strcmp(token.value, "+") == 0) ? OP_ADD : OP_SUBTRACT;
//             (*pos)++; // Consume operator
//             ASTNode *right = parse_comparison(tokens, pos, token_count);
//             if (right == NULL) {
//                 free_ast(left);
//                 return NULL;
//             }

//             // Create binary operation node
//             ASTNode *op_node = (ASTNode*)malloc(sizeof(ASTNode));
//             op_node->type = AST_BINARY_OP;
//             // DEBUG_PRINT("Add AST_BINARY_OP Node of %d", op_node->type);
//             op_node->data.operator = op_type;
//             op_node->left = left;
//             op_node->right = right;
//             op_node->nextblock = NULL;

//             left = op_node; // Update left node for the next iteration
//         } else {
//             break; // No more operators at this precedence level
//         }
//     }

//     return left;
// }

ASTNode* parse_expression(Token tokens[], int *pos, int token_count) {
    // comparisons have the lowest precedence after arithmetic
    return parse_comparison(tokens, pos, token_count);
}

ASTNode* parse_comparison(Token tokens[], int *pos, int token_count) {
    ASTNode *left = parse_additive(tokens, pos, token_count);
    if (left == NULL) {
        return NULL;
    }

    while (*pos < token_count && tokens[*pos].type == TOKEN_OPERATOR) {
        OperatorType op_type;
        int is_comparison = 0;

        if (strcmp(tokens[*pos].value, "<") == 0) {
            op_type = OP_LESS_THAN;
            is_comparison = 1;
        } else if (strcmp(tokens[*pos].value, ">") == 0) {
            op_type = OP_GREATER_THAN;
            is_comparison = 1;
        } else if (strcmp(tokens[*pos].value, "<=") == 0) {
            op_type = OP_LESS_EQUAL;
            is_comparison = 1;
        } else if (strcmp(tokens[*pos].value, ">=") == 0) {
            op_type = OP_GREATER_EQUAL;
            is_comparison = 1;
        } else if (strcmp(tokens[*pos].value, "==") == 0) {
            op_type = OP_EQUAL;
            is_comparison = 1;
        } else if (strcmp(tokens[*pos].value, "!=") == 0) {
            op_type = OP_NOT_EQUAL;
            is_comparison = 1;
        }

        if (!is_comparison) {
            // Not a comparison operator
            break;
        }

        (*pos)++; // consume operator
        ASTNode *right = parse_additive(tokens, pos, token_count);
        if (right == NULL) {
            free_ast(left);
            return NULL;
        }

        ASTNode *op_node = (ASTNode*)malloc(sizeof(ASTNode));
        op_node->type = AST_BINARY_OP;
        op_node->data.operator = op_type;
        op_node->left = left;
        op_node->right = right;

        left = op_node; // chain comparisons if multiple
    }

    return left;
}

ASTNode* parse_additive(Token tokens[], int *pos, int token_count) {
    ASTNode *left = parse_term(tokens, pos, token_count);
    if (left == NULL) {
        return NULL;
    }

    while (*pos < token_count && tokens[*pos].type == TOKEN_OPERATOR) {
        if (strcmp(tokens[*pos].value, "+") == 0 || strcmp(tokens[*pos].value, "-") == 0) {
            OperatorType op_type;
            if (strcmp(tokens[*pos].value, "+") == 0) {
                op_type = OP_ADD;
            } else {
                op_type = OP_SUBTRACT;
            }

            (*pos)++; // Consume operator
            ASTNode *right = parse_term(tokens, pos, token_count);
            if (right == NULL) {
                free_ast(left);
                return NULL;
            }

            ASTNode *op_node = (ASTNode*)malloc(sizeof(ASTNode));
            op_node->type = AST_BINARY_OP;
            op_node->data.operator = op_type;
            op_node->left = left;
            op_node->right = right;

            left = op_node; // Update left node
        } else {
            break;
        }
    }

    return left;
}

ASTNode* parse_term(Token tokens[], int *pos, int token_count) {
    ASTNode *left = parse_factor(tokens, pos, token_count);
    if (left == NULL) {
        return NULL;
    }

    while (*pos < token_count && tokens[*pos].type == TOKEN_OPERATOR) {
        if (strcmp(tokens[*pos].value, "*") == 0 || strcmp(tokens[*pos].value, "/") == 0) {
            OperatorType op_type;
            if (strcmp(tokens[*pos].value, "*") == 0) {
                op_type = OP_MULTIPLY;
            } else {
                op_type = OP_DIVIDE;
            }

            (*pos)++; // Consume operator
            ASTNode *right = parse_factor(tokens, pos, token_count);
            if (right == NULL) {
                free_ast(left);
                return NULL;
            }

            ASTNode *op_node = (ASTNode*)malloc(sizeof(ASTNode));
            op_node->type = AST_BINARY_OP;
            op_node->data.operator = op_type;
            op_node->left = left;
            op_node->right = right;

            left = op_node; // Continue chaining
        } else {
            break;
        }
    }

    return left;
}

// ASTNode* parse_factor(Token tokens[], int *pos, int token_count) {
//     if (*pos >= token_count) {
//         printf("Error: Unexpected end of input in factor\n");
//         return NULL;
//     }

//     Token token = tokens[*pos];

//     // Parenthesized expression
//     if (token.type == TOKEN_DELIMITER && token.value[0] == '(') {
//         (*pos)++;
//         ASTNode *node = parse_expression(tokens, pos, token_count);
//         if (node == NULL) {
//             return NULL;
//         }
//         // Expect ')'
//         if (*pos >= token_count || tokens[*pos].type != TOKEN_DELIMITER || tokens[*pos].value[0] != ')') {
//             printf("Error: Expected ')' after expression\n");
//             free_ast(node);
//             return NULL;
//         }
//         (*pos)++;
//         return node;
//     }

//     // Number or String literal
//     if (token.type == TOKEN_NUMBER || token.type == TOKEN_STRING) {
//         ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
//         node->type = AST_LITERAL;
//         node->left = node->right = NULL;
//         strcpy(node->data.string_value, token.value);
//         (*pos)++;
//         return node;
//     }

//     // Identifier or Function call
//     if (token.type == TOKEN_IDENTIFIER) {
//         char ident[MAX_TOKEN_LENGTH];
//         strcpy(ident, token.value);
//         (*pos)++;

//         // Check for function call
//         if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == '(') {
//             (*pos)++;
//             // Parse arguments
//             ASTNode *arg_list = NULL;
//             ASTNode **current = &arg_list;
//             while (*pos < token_count && !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
//                 ASTNode *arg = parse_expression(tokens, pos, token_count);
//                 if (arg == NULL) {
//                     free_ast(arg_list);
//                     return NULL;
//                 }
//                 *current = arg;
//                 current = &arg->right;

//                 // Check for comma
//                 if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ',') {
//                     (*pos)++;
//                 }
//             }

//             // Expect ')'
//             if (*pos >= token_count || !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
//                 printf("Error: Expected ')' after function call arguments\n");
//                 free_ast(arg_list);
//                 return NULL;
//             }
//             (*pos)++;

//             // Create function call node
//             ASTNode *call_node = (ASTNode*)malloc(sizeof(ASTNode));
//             call_node->type = AST_FUNCTION_CALL;
//             call_node->left = call_node->right = NULL;
//             strcpy(call_node->data.func_call.name, ident);
//             call_node->data.func_call.arguments = arg_list;
//             return call_node;
//         } else {
//             // Just a variable (identifier)
//             ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
//             node->type = AST_IDENTIFIER;
//             node->left = node->right = NULL;
//             strcpy(node->data.identifier, ident);
//             return node;
//         }
//     }

//     printf("Error: Unexpected token '%s' in factor\n", token.value);
//     return NULL;
// }

ASTNode* parse_factor(Token tokens[], int *pos, int token_count) {
    if (*pos >= token_count) {
        printf("Error: Unexpected end of input in factor\n");
        return NULL;
    }

    Token token = tokens[*pos];

    // Parenthesized expression
    if (token.type == TOKEN_DELIMITER && token.value[0] == '(') {
        (*pos)++;
        ASTNode *node = parse_expression(tokens, pos, token_count);
        if (node == NULL) return NULL;

        // Expect ')'
        if (*pos >= token_count || tokens[*pos].type != TOKEN_DELIMITER || tokens[*pos].value[0] != ')') {
            printf("Error: Expected ')' after expression\n");
            free_ast(node);
            return NULL;
        }
        (*pos)++;
        return node;
    }

    // Number or String literal
    if (token.type == TOKEN_NUMBER || token.type == TOKEN_STRING) {
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_LITERAL;
        node->left = node->right = NULL;
        strcpy(node->data.string_value, token.value);
        (*pos)++;
        return node;
    }

    // Identifier or Function call or Array access
    if (token.type == TOKEN_IDENTIFIER) {
        char ident[MAX_TOKEN_LENGTH];
        strcpy(ident, token.value);
        (*pos)++;

        // Check for function call (identifier followed by '(')
        if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == '(') {
            (*pos)++;
            // Parse arguments
            ASTNode *arg_list = NULL;
            ASTNode **current = &arg_list;
            while (*pos < token_count && !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
                ASTNode *arg = parse_expression(tokens, pos, token_count);
                if (arg == NULL) {
                    free_ast(arg_list);
                    return NULL;
                }
                *current = arg;
                current = &arg->right;

                // Check for comma
                if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ',') {
                    (*pos)++;
                }
            }

            // Expect ')'
            if (*pos >= token_count || !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
                printf("Error: Expected ')' after function call arguments\n");
                free_ast(arg_list);
                return NULL;
            }
            (*pos)++;

            // Create function call node
            ASTNode *call_node = (ASTNode*)malloc(sizeof(ASTNode));
            call_node->type = AST_FUNCTION_CALL;
            call_node->left = call_node->right = NULL;
            strcpy(call_node->data.func_call.name, ident);
            call_node->data.func_call.arguments = arg_list;
            return call_node;
        }

        // Check for array access (identifier followed by '[')
        if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == '[') {
            (*pos)++;
            // Parse the index expression inside []
            ASTNode *index_expr = parse_expression(tokens, pos, token_count);
            if (index_expr == NULL) {
                return NULL;
            }

            // Expect ']'
            if (*pos >= token_count || tokens[*pos].type != TOKEN_DELIMITER || tokens[*pos].value[0] != ']') {
                printf("Error: Expected ']' after array index\n");
                free_ast(index_expr);
                return NULL;
            }
            (*pos)++;

            // Create AST_ARRAY_ACCESS node
            ASTNode *access_node = (ASTNode*)malloc(sizeof(ASTNode));
            access_node->type = AST_ARRAY_ACCESS;
            strcpy(access_node->data.identifier, ident);
            access_node->left = index_expr;
            access_node->right = NULL;
            return access_node;
        }

        // Just a variable (identifier)
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_IDENTIFIER;
        node->left = node->right = NULL;
        strcpy(node->data.identifier, ident);
        return node;
    }

    printf("Error: Unexpected token '%s' in factor\n", token.value);
    return NULL;
}

ASTNode* parse_print_statement(Token tokens[], int *pos, int token_count) {
    if (tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "print") == 0) {
        (*pos)++;
        if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == '(') {
            (*pos)++;
            // Parse the expression inside the parentheses
            ASTNode *expr = parse_expression(tokens, pos, token_count);
            if (expr == NULL) {
                printf("Error: Expected expression after 'print('\n");
                return NULL;
            }
            if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')') {
                (*pos)++;
                // Successfully parsed 'print' statement
                ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
                node->type = AST_PRINT;
                // DEBUG_PRINT("Add AST_PRINT Node of %d", node->type);
                node->left = expr; // Expression to print
                node->right = NULL;
                node->nextblock = NULL;
                return node;
            } else {
                printf("Error: Expected ')' after expression\n");
                free_ast(expr);
                return NULL;
            }
        } else {
            printf("Error: Expected '(' after 'print'\n");
            return NULL;
        }
    }
    return NULL;
}

ASTNode* parse_if_statement(Token tokens[], int *pos, int token_count) {
    if (tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "if") == 0) {
        (*pos)++;
        // Parse the condition expression
        ASTNode *condition = parse_expression(tokens, pos, token_count);
        if (condition == NULL) {
            printf("Error: Expected condition after 'if'\n");
            return NULL;
        }
        // Parse the 'then' branch
        ASTNode *then_branch = parse_block(tokens, pos, token_count);
        if (then_branch == NULL) {
            printf("Error: Expected block after 'if' condition\n");
            free_ast(condition);
            return NULL;
        }
        // Check for 'else' clause
        ASTNode *else_branch = NULL;
        if (*pos < token_count && tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "else") == 0) {
            (*pos)++;
            else_branch = parse_block(tokens, pos, token_count);
            if (else_branch == NULL) {
                printf("Error: Expected block after 'else'\n");
                free_ast(condition);
                free_ast(then_branch);
                return NULL;
            }
        }
        // Expect 'end' keyword
        if (*pos < token_count && tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "end") == 0) {
            (*pos)++;
            // Create the if statement node
            ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
            node->type = AST_IF_STATEMENT;
            // DEBUG_PRINT("Add AST_IF_STATEMENT Node of %d", node->type);
            node->left = NULL;
            node->right = NULL;
            node->nextblock = NULL;
            node->data.if_stmt.condition = condition;
            node->data.if_stmt.then_branch = then_branch;
            node->data.if_stmt.else_branch = else_branch;
            return node;
        } else {
            printf("Error: Expected 'end' after 'if' statement\n");
            free_ast(condition);
            free_ast(then_branch);
            if (else_branch) free_ast(else_branch);
            return NULL;
        }
    }
    return NULL;
}

ASTNode* parse_for_statement(Token tokens[], int *pos, int token_count) {
    // Expect 'for'
    if (*pos < token_count && tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "for") == 0) {
        (*pos)++;

        // Expect identifier for loop variable
        if (*pos >= token_count || tokens[*pos].type != TOKEN_IDENTIFIER) {
            printf("Error: Expected identifier after 'for'\n");
            return NULL;
        }
        char loop_var[MAX_TOKEN_LENGTH];
        strcpy(loop_var, tokens[*pos].value);
        (*pos)++;

        // Expect 'in'
        if (*pos >= token_count || tokens[*pos].type != TOKEN_KEYWORD || strcmp(tokens[*pos].value, "in") != 0) {
            printf("Error: Expected 'in' after loop variable\n");
            return NULL;
        }
        (*pos)++;

        // Parse expression that should yield an array
        ASTNode *expr = parse_expression(tokens, pos, token_count);
        if (expr == NULL) {
            printf("Error: Expected expression after 'in'\n");
            return NULL;
        }

        // Parse the block of statements until 'end'
        ASTNode *body = parse_block(tokens, pos, token_count);
        if (body == NULL) {
            printf("Error: Expected block after 'for' statement\n");
            free_ast(expr);
            return NULL;
        }

        // Expect 'end'
        if (*pos >= token_count || tokens[*pos].type != TOKEN_KEYWORD || strcmp(tokens[*pos].value, "end") != 0) {
            printf("Error: Expected 'end' after 'for' block\n");
            free_ast(expr);
            free_ast(body);
            return NULL;
        }
        (*pos)++;

        // Create the for statement node
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_FOR_STATEMENT;
        // DEBUG_PRINT("Add AST_FOR_STATEMENT Node of %d", node->type);
        node->left = node->right = NULL;
        node->nextblock = NULL;
        strcpy(node->data.for_stmt.loop_var, loop_var);
        node->data.for_stmt.expression = expr;
        node->data.for_stmt.body = body;
        return node;
    }

    return NULL; // Not a for statement
}

ASTNode* parse_while_statement(Token tokens[], int *pos, int token_count) {
    if (*pos < token_count && tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "while") == 0) {
        (*pos)++;

        // Parse the condition expression
        ASTNode *condition = parse_expression(tokens, pos, token_count);
        if (condition == NULL) {
            printf("Error: Expected condition after 'while'\n");
            return NULL;
        }

        // Parse the block of statements until 'end'
        ASTNode *body = parse_block(tokens, pos, token_count);
        if (body == NULL) {
            printf("Error: Expected block after 'while' condition\n");
            free_ast(condition);
            return NULL;
        }

        // Expect 'end'
        if (*pos >= token_count || tokens[*pos].type != TOKEN_KEYWORD || strcmp(tokens[*pos].value, "end") != 0) {
            printf("Error: Expected 'end' after 'while' block\n");
            free_ast(condition);
            free_ast(body);
            return NULL;
        }
        (*pos)++;

        // Create the while statement node
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_WHILE_STATEMENT;
        // DEBUG_PRINT("Add AST_WHILE_STATEMENT Node of %d", node->type);
        node->left = node->right = NULL;
        node->nextblock = NULL;
        node->data.while_stmt.condition = condition;
        node->data.while_stmt.body = body;
        return node;
    }

    return NULL; // Not a while statement
}

ASTNode* parse_function_definition(Token tokens[], int *pos, int token_count) {
    if (*pos < token_count && tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "def") == 0) {
        (*pos)++;
        // Expect function name
        if (*pos >= token_count || tokens[*pos].type != TOKEN_IDENTIFIER) {
            printf("Error: Expected function name after 'def'\n");
            return NULL;
        }

        char func_name[MAX_TOKEN_LENGTH];
        strcpy(func_name, tokens[*pos].value);
        (*pos)++;

        // Parse parameter list
        // Expect '('
        if (*pos >= token_count || tokens[*pos].type != TOKEN_DELIMITER || tokens[*pos].value[0] != '(') {
            printf("Error: Expected '(' after function name\n");
            return NULL;
        }
        (*pos)++;

        // Parse parameters (identifiers separated by commas)
        ASTNode *param_list = NULL;
        ASTNode **current = &param_list;
        while (*pos < token_count && !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
            if (tokens[*pos].type == TOKEN_IDENTIFIER) {
                ASTNode *param = (ASTNode*)malloc(sizeof(ASTNode));
                param->type = AST_IDENTIFIER;
                // DEBUG_PRINT("Add AST_IDENTIFIER Node of %d", param->type);
                strcpy(param->data.identifier, tokens[*pos].value);
                param->left = param->right = NULL;
                param->nextblock = NULL;
                (*pos)++;
                *current = param;
                current = &param->right;

                // Check for comma
                if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ',') {
                    (*pos)++;
                    continue;
                }
            } else {
                printf("Error: Expected parameter name or ')' in function definition\n");
                free_ast(param_list);
                return NULL;
            }
        }

        // Expect ')'
        if (*pos >= token_count || tokens[*pos].type != TOKEN_DELIMITER || tokens[*pos].value[0] != ')') {
            printf("Error: Expected ')' after parameters\n");
            free_ast(param_list);
            return NULL;
        }
        (*pos)++;

        // Parse the body block until 'end'
        ASTNode *body = parse_block(tokens, pos, token_count);
        if (body == NULL) {
            printf("Error: Expected block after function definition\n");
            free_ast(param_list);
            return NULL;
        }

        // Expect 'end'
        if (*pos >= token_count || tokens[*pos].type != TOKEN_KEYWORD || strcmp(tokens[*pos].value, "end") != 0) {
            printf("Error: Expected 'end' after function body\n");
            free_ast(param_list);
            free_ast(body);
            return NULL;
        }
        (*pos)++;

        // Create function definition node
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_FUNCTION_DEFINITION;
        // DEBUG_PRINT("Add AST_FUNCTION_DEFINITION Node of %d", node->type);
        node->left = node->right = NULL;
        node->nextblock = NULL;
        strcpy(node->data.func_def.name, func_name);
        node->data.func_def.parameters = param_list;
        node->data.func_def.body = body;

        // After creating the AST_FUNCTION_DEFINITION node in parse_function_definition:
        if (function_count < MAX_FUNCTIONS) {
            strcpy(functions[function_count].name, node->data.func_def.name);
            functions[function_count].parameters = node->data.func_def.parameters;
            functions[function_count].body = node->data.func_def.body;
            function_count++;
        } else {
            printf("Error: Too many functions defined\n");
        }
        return node;
    }

    return NULL;
}

ASTNode* parse_return_statement(Token tokens[], int *pos, int token_count) {
    if (*pos < token_count && tokens[*pos].type == TOKEN_KEYWORD && strcmp(tokens[*pos].value, "return") == 0) {
        (*pos)++;
        // Parse expression after return
        ASTNode *expr = parse_expression(tokens, pos, token_count);
        if (expr == NULL) {
            // No expression means return 0
            // We'll handle this by setting expr to a literal 0 node
            ASTNode *zero_node = (ASTNode*)malloc(sizeof(ASTNode));
            zero_node->type = AST_LITERAL;
            // DEBUG_PRINT("Add AST_LITERAL Node of %d", zero_node->type);
            zero_node->left = zero_node->right = NULL;
            zero_node->nextblock = NULL;
            strcpy(zero_node->data.string_value, "0");
            expr = zero_node;
        }
        ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
        node->type = AST_RETURN_STATEMENT;
        // DEBUG_PRINT("Add AST_RETURN_STATEMENT Node of %d", node->type);
        node->left = node->right = NULL;
        node->nextblock = NULL;
        node->data.ret_stmt.expression = expr;
        return node;
    }
    return NULL;
}

ASTNode* parse_function_call(Token tokens[], int *pos, int token_count) {
    if (tokens[*pos].type == TOKEN_IDENTIFIER) {
        if ((*pos + 1) < token_count &&
        tokens[*pos + 1].type == TOKEN_DELIMITER &&
        tokens[*pos + 1].value[0] == '(') {
            // Function call 
            char ident[MAX_TOKEN_LENGTH];
            strcpy(ident, tokens[*pos].value);
            (*pos)++;

            // Parse arguments
            (*pos)++;
            ASTNode *arg_list = NULL;
            ASTNode **current = &arg_list;
            while (*pos < token_count && !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
                ASTNode *arg = parse_expression(tokens, pos, token_count);
                if (arg == NULL) {
                    printf("Error: Expected expression in function call arguments\n");
                    free_ast(arg_list);
                    return NULL;
                }
                *current = arg;
                current = &arg->right;

                // Check for comma
                if (*pos < token_count && tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ',') {
                    (*pos)++;
                }
            }

            // Expect ')'
            if (*pos >= token_count || !(tokens[*pos].type == TOKEN_DELIMITER && tokens[*pos].value[0] == ')')) {
                printf("Error: Expected ')' after function call arguments\n");
                //free_ast(arg_list);
                return NULL;
            }
            (*pos)++;

            // Create function call node
            ASTNode *call_node = (ASTNode*)malloc(sizeof(ASTNode));
            call_node->type = AST_FUNCTION_CALL;
            // DEBUG_PRINT("Add AST_FUNCTION_CALL Node of %d", call_node->type);
            call_node->left = call_node->right = NULL;
            call_node->nextblock = NULL;
            strcpy(call_node->data.func_call.name, ident);
            call_node->data.func_call.arguments = arg_list;
            //call_node->data.func_call.arguments = NULL;
            return call_node;
        }
    }

    return NULL; // Not a function call
}

ASTNode* parse_statement(Token tokens[], int *pos, int token_count) {
    ASTNode *node = NULL;

    if (*pos >= token_count) return NULL;
    // DEBUG_PRINT(">>> %s", tokens[*pos].value);
    // Check if it's a for statement
    node = parse_for_statement(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Try to parse an 'if' statement
    node = parse_if_statement(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Try while statement
    node = parse_while_statement(tokens, pos, token_count);
    if (node != NULL) return node;

    // Try to parse a function definition
    node = parse_function_definition(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Try to parse a return in a function
    node = parse_return_statement(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Try to parse a function call
    // Note this must be tried before parse_assignment_statement() as both
    // start with an TOKEN_IDENTIFIER
    node = parse_function_call(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Try to parse a 'print' statement
    node = parse_print_statement(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Try to parse an assignment statement
    node = parse_assignment_statement(tokens, pos, token_count);
    if (node != NULL) {
        return node;
    }

    // Unrecognized statement
    printf("Error: Unrecognized statement starting with '%s'\n", tokens[*pos].value);
    return NULL;
}

ASTNode* parse_block(Token tokens[], int *pos, int token_count) {
    ASTNode *statements = NULL; // This will be a linked list of statements
    ASTNode **current = &statements;

    while (*pos < token_count) {
        if (tokens[*pos].type == TOKEN_KEYWORD && 
            (strcmp(tokens[*pos].value, "else") == 0 || strcmp(tokens[*pos].value, "end") == 0)) {
            // End of the block
            break;
        }

        ASTNode *stmt = parse_statement(tokens, pos, token_count);
        if (stmt == NULL) {
            printf("Error: Failed to parse statement in block\n");
            free_ast(statements);
            return NULL;
        }

        *current = stmt;
        //current = &(stmt->right); // Use the 'right' pointer to link statements
        current = &(stmt->nextblock); // Use the 'nextblock' pointer to link statements
    }

    return statements;
}

void execute_if_statement(ASTNode *node) {
    if (node == NULL || node->type != AST_IF_STATEMENT) return;

    EvalResult condition_result;
    if (!evaluate_expression(node->data.if_stmt.condition, &condition_result, EVAL_ARITHMETIC)) {
        printf("Error: Failed to evaluate condition in if statement\n");
        return;
    }

    // Interpret any non-zero number as true
    int condition_true = 0;
    if (condition_result.type == RESULT_NUMBER) {
        condition_true = (condition_result.number_value != 0);
    } else if (condition_result.type == RESULT_STRING) {
        // Empty string is false, non-empty string is true
        condition_true = (strlen(condition_result.string_value) > 0);
    } else {
        printf("Error: Invalid condition type in if statement\n");
        return;
    }

    if (condition_true) {
        // Execute the then branch
        execute_block(node->data.if_stmt.then_branch);
    } else if (node->data.if_stmt.else_branch != NULL) {
        // Execute the else branch
        execute_block(node->data.if_stmt.else_branch);
    }
}

void execute_for_statement(ASTNode *node) {
    if (node == NULL || node->type != AST_FOR_STATEMENT) return;
    // DEBUG_PRINT("execute_for_statement");
    // Evaluate the expression to get the array
    EvalResult result;
    if (!evaluate_expression(node->data.for_stmt.expression, &result, EVAL_PRINT)) {
        printf("Error: Failed to evaluate expression in for statement\n");
        return;
    }
    // DEBUG_PRINT("result.type %d", result.type);
    AssocArray temp_array;
    init_assoc_array(&temp_array);

    if (result.type == RESULT_ASSOC_ARRAY) {
        // Use the existing associative array
        // We'll just reference result.array_value directly
    } else if (result.type == RESULT_STRING) {
        // Wrap the single string in an associative array
        set_assoc_array_value(&temp_array, "", result.string_value);
        result.type = RESULT_ASSOC_ARRAY;
        result.array_value = &temp_array;
    } else if (result.type == RESULT_NUMBER) {
        // Convert the number to a string and wrap it
        char num_str[MAX_TOKEN_LENGTH];
        snprintf(num_str, MAX_TOKEN_LENGTH, "%g", result.number_value);
        set_assoc_array_value(&temp_array, "", num_str);
        result.type = RESULT_ASSOC_ARRAY;
        result.array_value = &temp_array;
    } else {
        printf("Error: For loop expression must be a variable or array\n");
        return;
    }

    // Now we have an associative array in result.array_value
    AssocArray *array = result.array_value;

    // Iterate over each key-value pair in the array
    for (int i = 0; i < array->size; i++) {
        // Assign the value to the loop variable
        set_variable_value(node->data.for_stmt.loop_var, array->pairs[i].key, array->pairs[i].value);

        // Execute the body
        // DEBUG_PRINT("About to execute_block");
        execute_block(node->data.for_stmt.body);

        // Clear the loop_var before each iteration
        clear_variable_assoc_array(node->data.for_stmt.loop_var);
    }

    // If we used a temporary array, free it
    if (result.array_value == &temp_array) {
        free_assoc_array(&temp_array);
    }
}

void execute_while_statement(ASTNode *node) {
    if (node == NULL || node->type != AST_WHILE_STATEMENT) return;

    while (1) {
        EvalResult condition_result;
        // Evaluate condition in arithmetic context (to handle numbers easily)
        if (!evaluate_expression(node->data.while_stmt.condition, &condition_result, EVAL_ARITHMETIC)) {
            printf("Error: Failed to evaluate condition in while statement\n");
            return;
        }

        int condition_true = 0;
        if (condition_result.type == RESULT_NUMBER) {
            condition_true = (condition_result.number_value != 0);
        } else if (condition_result.type == RESULT_STRING) {
            // Empty string = false, non-empty = true
            condition_true = (strlen(condition_result.string_value) > 0);
        } else {
            // Associative arrays, or other types: treat as true if non-empty?
            // For simplicity, treat arrays as true if they have at least one element.
            // If you want different logic, define it here.
            if (condition_result.type == RESULT_ASSOC_ARRAY) {
                condition_true = (condition_result.array_value->size > 0);
            } else {
                condition_true = 0; // Treat as false
            }
        }

        if (!condition_true) {
            break; // Exit loop if condition is false
        }

        // Execute the body block
        execute_block(node->data.while_stmt.body);
    }
}

void execute_block(ASTNode *node) {
    while (node != NULL) {
        // DEBUG_PRINT("execute_block type %d", node->type);
        // DEBUG_PRINT("execute_block left %p", node->left);
        // DEBUG_PRINT("execute_block right %p", node->right);
        execute_ast(node);
        //node = node->right; // Move to the next statement in the block
        node = node->nextblock; // Move to the next statement in the block
    }
}

void execute_ast(ASTNode *node)
{
    if (node == NULL)
        return;

    // printf("execute_ast %d\n", node->type);

    switch (node->type) {
        case AST_PRINT:
            // Evaluate the expression and print the result
            evaluate_and_print(node->left);
            break;
        case AST_ASSIGNMENT:
            execute_assignment(node);
            break;
        case AST_IF_STATEMENT:
            execute_if_statement(node);
            break;
        case AST_BLOCK:
            execute_block(node);
            break;
        case AST_FOR_STATEMENT:
            execute_for_statement(node);
            break;      
        case AST_WHILE_STATEMENT:
            execute_while_statement(node);
            break;
        case AST_FUNCTION_DEFINITION:
            // Already handled at parse time; no run-time needed unless we allow redef.
            break;
        case AST_FUNCTION_CALL: {
            FunctionReturn call_res = execute_function_call(node);
            // If called as a statement, ignore return value
            break;
        }
        default:
            printf("Error: While executing the AST - Unknown AST node type (%d)\n", node->type);
            break;
    }
}

FunctionReturn execute_ast_with_return(ASTNode *node) {
    FunctionReturn result = {0};
    if (node == NULL) return result;

    switch (node->type) {
        // handle statements
        case AST_PRINT:
            evaluate_and_print(node->left);
            break;
        case AST_ASSIGNMENT:
            execute_assignment(node);
            break;
        // case AST_IF_STATEMENT:
        //     return execute_if_statement_with_return(node);
        // case AST_FOR_STATEMENT:
        //     return execute_for_statement_with_return(node);
        // case AST_WHILE_STATEMENT:
        //     return execute_while_statement_with_return(node);
        case AST_FUNCTION_DEFINITION:
            // Already handled at parse time; no run-time needed unless we allow redef.
            break;
        case AST_FUNCTION_CALL: {
            FunctionReturn call_res = execute_function_call(node);
            // If called as a statement, ignore return value
            break;
        }
        case AST_RETURN_STATEMENT: {
            // Evaluate the return expression
            EvalResult val;
            if (!evaluate_expression(node->data.ret_stmt.expression, &val, EVAL_ARITHMETIC)) {
                printf("Error: Failed to evaluate return expression\n");
                // return 0 by default
                result.has_return = 1;
                //result.return_is_number = 1;
                result.type = RESULT_NUMBER;
                result.number_value = 0;
                //strcpy(result.return_value, "0");
                return result;
            }
            result.has_return = 1;
            if (val.type == RESULT_NUMBER) {
                //result.return_is_number = 1;
                result.type = RESULT_NUMBER;
                //snprintf(result.return_value, MAX_TOKEN_LENGTH, "%g", val.number_value);
                result.number_value = val.number_value;
            } else if (val.type == RESULT_STRING) {
                //result.return_is_number = 0;
                result.type = RESULT_STRING;
                strcpy(result.string_value, val.string_value);
            } else if (val.type == RESULT_ASSOC_ARRAY) {
                //result.return_is_number = 0;
                result.type = RESULT_ASSOC_ARRAY;
                result.array_value = (AssocArray *) malloc(sizeof(AssocArray)); // Where does this get freed?
                duplicate_assoc_array(result.array_value, val.array_value);
            } else {
                // If other, decide how to return them
                // For simplicity, return "0"
                // result.return_is_number = 1;
                // strcpy(result.return_value, "0");
                result.type = RESULT_NUMBER;
                result.number_value = 0;
            }
            return result;
        }
        default:
            printf("Error: While executing the AST (with return) - Unknown AST node type\n");
            DEBUG_PRINT("node->type %d", node->type);
            break;
    }
    return result;
}

int evaluate_expression(ASTNode *node, EvalResult *result, EvalContext context) {
    if (node == NULL) return 0;
DEBUG_PRINT("evaluate_expression node->type %d", node->type);
    switch (node->type) {
        case AST_LITERAL:
            // Check if it's a number
            if (isdigit(node->data.string_value[0]) || (node->data.string_value[0] == '-' && isdigit(node->data.string_value[1]))) {
                result->type = RESULT_NUMBER;
                result->number_value = atof(node->data.string_value);
            } else {
                result->type = RESULT_STRING;
                strcpy(result->string_value, node->data.string_value);
            }
            return 1;
        case AST_IDENTIFIER: {
            Variable *var = get_variable(node->data.identifier);
            if (var != NULL) {
                if (context == EVAL_ARITHMETIC) {
                    // if (var->array.size == 1 && strcmp(var->array.pairs[0].key, "") == 0) {
                    //     // Always use the default key "" for arithmetic expressions
                    if (var->array.size == 1) {
                        // Always use the only key
                        // char *value = get_assoc_array_value(&var->array, "");
                        char *value = get_first_assoc_array_value(&var->array);
                        if (value != NULL) {
                            // Determine if value is a number
                            if (isdigit(value[0]) || (value[0] == '-' && isdigit(value[1]))) {
                                result->type = RESULT_NUMBER;
                                result->number_value = atof(value);
                            } else {
                                result->type = RESULT_STRING;
                                strcpy(result->string_value, value);
                            }
                            return 1;
                        } else {
                            printf("Error: Variable '%s' has no default value and cannot be used in expressions\n", node->data.identifier);
                            return 0;
                        }
                    } else {
                        // Return the associative array
                        result->type = RESULT_ASSOC_ARRAY;
                        result->array_value = &var->array;
                        return 1;                        
                    }
                } else if (context == EVAL_PRINT) {
                    // For printing, return the entire associative array or the default value
                    // if (var->array.size == 1 && strcmp(var->array.pairs[0].key, "") == 0) {
                    //     // Only default key, treat as simple variable
                    if (var->array.size == 1) {
                        // Only item, treat as simple variable
                        result->type = RESULT_STRING;
                        strcpy(result->string_value, var->array.pairs[0].value);
                    } else {
                        // Multiple keys, return the associative array
                        result->type = RESULT_ASSOC_ARRAY;
                        result->array_value = &var->array;
                    }
                    return 1;
                }
            } else {
                printf("Error: Undefined variable '%s'\n", node->data.identifier);
                return 0;
            }
        }
        
        case AST_ARRAY_ACCESS: {
            EvalResult key_result;
            if (!evaluate_expression(node->left, &key_result, EVAL_ARITHMETIC)) {
                printf("Error: Failed to evaluate array index\n");
                DEBUG_PRINT("Error: Failed to evaluate array index");
                return 0;
            }
            if (key_result.type != RESULT_STRING && key_result.type != RESULT_NUMBER) {
                printf("Error: Array index must be a string or number\n");
                return 0;
            }

            // Convert numeric index to string if necessary
            char key[MAX_TOKEN_LENGTH];
            if (key_result.type == RESULT_NUMBER) {
                snprintf(key, MAX_TOKEN_LENGTH, "%g", key_result.number_value);
            } else {
                strcpy(key, key_result.string_value);
            }

            Variable *var = get_variable(node->data.identifier);
            if (var == NULL) {
                printf("Error: Undefined variable '%s'\n", node->data.identifier);
                return 0;
            }

            char *value = get_assoc_array_value(&var->array, key);
            if (value != NULL) {
                // Determine if value is a number
                if (isdigit(value[0]) || (value[0] == '-' && isdigit(value[1]))) {
                    result->type = RESULT_NUMBER;
                    result->number_value = atof(value);
                } else {
                    result->type = RESULT_STRING;
                    strcpy(result->string_value, value);
                }
                return 1;
            } else {
                printf("Error: Key '%s' not found in variable '%s'\n", key, node->data.identifier);
                return 0;
            }
        }

        case AST_BINARY_OP: {
            EvalResult left_result, right_result;
            // For relational operators, we need to determine the context
            EvalContext op_context = (node->data.operator == OP_ADD || node->data.operator == OP_SUBTRACT ||
                                    node->data.operator == OP_MULTIPLY || node->data.operator == OP_DIVIDE)
                                    ? EVAL_ARITHMETIC : context;

            if (!evaluate_expression(node->left, &left_result, op_context)) {
                return 0;
            }
            if (!evaluate_expression(node->right, &right_result, op_context)) {
                return 0;
            }

            // Handle arithmetic and relational operators
            if (left_result.type == RESULT_NUMBER && right_result.type == RESULT_NUMBER) {
                switch (node->data.operator) {
                    case OP_ADD:
                        result->type = RESULT_NUMBER;
                        result->number_value = left_result.number_value + right_result.number_value;
                        return 1;
                    case OP_SUBTRACT:
                        result->type = RESULT_NUMBER;
                        result->number_value = left_result.number_value - right_result.number_value;
                        return 1;
                    case OP_MULTIPLY:
                        result->type = RESULT_NUMBER;
                        result->number_value = left_result.number_value * right_result.number_value;
                        return 1;
                    case OP_DIVIDE:
                        result->type = RESULT_NUMBER;
                        result->number_value = left_result.number_value / right_result.number_value;
                        return 1;
                    case OP_LESS_THAN:
                        result->type = RESULT_NUMBER;
                        result->number_value = (left_result.number_value < right_result.number_value) ? 1 : 0;
                        return 1;
                    case OP_GREATER_THAN:
                        result->type = RESULT_NUMBER;
                        result->number_value = (left_result.number_value > right_result.number_value) ? 1 : 0;
                        return 1;
                    case OP_EQUAL:
                        result->type = RESULT_NUMBER;
                        result->number_value = (left_result.number_value == right_result.number_value) ? 1 : 0;
                        return 1;
                    case OP_NOT_EQUAL:
                        result->type = RESULT_NUMBER;
                        result->number_value = (left_result.number_value != right_result.number_value) ? 1 : 0;
                        return 1;
                    case OP_LESS_EQUAL:
                        result->type = RESULT_NUMBER;
                        result->number_value = (left_result.number_value <= right_result.number_value) ? 1 : 0;
                        return 1;
                    case OP_GREATER_EQUAL:
                        result->type = RESULT_NUMBER;
                        result->number_value = (left_result.number_value >= right_result.number_value) ? 1 : 0;
                        return 1;
                    // Handle other operators...
                    default:
                        printf("Error: Unsupported operator\n");
                        return 0;
                }
            } else {
                printf("Error: Both operands must be numbers for arithmetic or relational operations\n");
                return 0;
            }
        }
        case AST_FUNCTION_CALL: {
            FunctionReturn call_res = execute_function_call(node);
            // TODO the return value is IMPORTANT!
            if(call_res.has_return == 0) {
                result->type = RESULT_NUMBER;
                result->number_value = 0;
            } else {
                if(call_res.type == RESULT_NUMBER) {
                    result->type = RESULT_NUMBER;
                    result->number_value = call_res.number_value;
                } else if (call_res.type == RESULT_ASSOC_ARRAY) {
                    result->type = RESULT_ASSOC_ARRAY;
                    result->array_value = call_res.array_value;
                } else {
                    result->type = RESULT_STRING;
                    strcpy(result->string_value, call_res.string_value);
                }
            }
            return 1;
            break;
        }

        default:
            printf("Error: While evaluating expression - Unknown AST node type\n");
            DEBUG_PRINT("DEBUG: While evaluating expression - Unknown AST node type (%d)\n", node->type);
            break;
    }
}

void clear_variable_assoc_array(const char *name) {
    Variable *var = NULL;
    // Check if variable exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            var = &variables[i];
            break;
        }
    }

    if (var != NULL) {
        free_assoc_array(&var->array);
        init_assoc_array(&var->array);
    }

}

void set_variable_assoc_array(const char *name, AssocArray *array_value) {
    Variable *var = NULL;
    // Check if variable exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            var = &variables[i];
            break;
        }
    }

    if (var == NULL) {
        // Create new variable
        if (variable_count >= MAX_VARIABLES) {
            printf("Error: Maximum number of variables reached\n");
            return;
        }
        strcpy(variables[variable_count].name, name);
        init_assoc_array(&variables[variable_count].array);
        var = &variables[variable_count];
        variable_count++;
    } else {
        // Free existing array
        free_assoc_array(&var->array);
        init_assoc_array(&var->array);
    }

    // Copy the associative array
    for (int i = 0; i < array_value->size; i++) {
        set_assoc_array_value(&var->array, array_value->pairs[i].key, array_value->pairs[i].value);
    }
}

#define MAX_SCOPES 100

// Global variables tracking scopes
// Variable variables[MAX_VARIABLES];
// int variable_count = 0;
Variable scope_variables_stack[MAX_SCOPES][MAX_VARIABLES];
int scope_count_stack[MAX_SCOPES];
int scope_depth = 0;

// push_scope saves the current variable_count on a stack
void push_scope() {
    if (scope_depth >= MAX_SCOPES) {
        printf("Error: Scope stack overflow\n");
        return;
    }
    // Record the current variables
    memcpy(&scope_variables_stack[scope_depth], variables, sizeof(Variable) * MAX_VARIABLES);
    scope_count_stack[scope_depth++] = variable_count;
    variable_count = 0;
}

// pop_scope restores variable_count to the value it had at the last push_scope
void pop_scope() {
    if (scope_depth <= 0) {
        printf("Error: Scope stack underflow\n");
        return;
    }

    int variable_count = scope_count_stack[--scope_depth];
    memcpy(variables, &scope_variables_stack[scope_depth], sizeof(Variable) * MAX_VARIABLES);
}

void execute_assignment(ASTNode *node) {
    if (node == NULL || node->type != AST_ASSIGNMENT) return;
DEBUG_PRINT("execute_assignment");
    ASTNode *target = node->left;
    ASTNode *expr = node->right;

    // Evaluate the expression
    EvalResult result;
    if (!evaluate_expression(expr, &result, EVAL_ARITHMETIC)) {
        printf("Error: Failed to evaluate expression in assignment\n");
        DEBUG_PRINT("Error: Failed to evaluate expression in assignment");
        return;
    }

    DEBUG_PRINT("target->type %d result.type %d", target->type, result.type);
    if (target->type == AST_IDENTIFIER) {
        // Simple variable assignment
        if (result.type == RESULT_STRING) {
            set_variable_value(target->data.identifier, NULL, result.string_value);
        } else if (result.type == RESULT_NUMBER) {
            char num_str[MAX_TOKEN_LENGTH];
            // DEBUG_PRINT("result.number_value %f", result.number_value);
            snprintf(num_str, MAX_TOKEN_LENGTH, "%g", result.number_value);
            set_variable_value(target->data.identifier, NULL, num_str);
        } else if (result.type == RESULT_ASSOC_ARRAY) {
            set_variable_assoc_array(target->data.identifier, result.array_value);
        }
    } else if (target->type == AST_ARRAY_ACCESS) {
        // Array element assignment
        EvalResult key_result;
        if (!evaluate_expression(target->left, &key_result, EVAL_PRINT)) {
            printf("Error: Failed to evaluate array index\n");
            DEBUG_PRINT("Error: Failed to evaluate array index");
            return;
        }
        if ( (key_result.type != RESULT_STRING) && (key_result.type != RESULT_NUMBER) ){
            printf("Assignment Error: Array index must be a string or number\n");
            return;
        }
        char key_str[MAX_TOKEN_LENGTH];
        if (key_result.type == RESULT_NUMBER) {
            snprintf(key_str, MAX_TOKEN_LENGTH, "%g", key_result.number_value);
        } else {
            strcpy(key_str, key_result.string_value);
        }
        if (result.type == RESULT_STRING) {
            set_variable_value(target->data.identifier, key_str, result.string_value);
        } else if (result.type == RESULT_NUMBER) {
            char num_str[MAX_TOKEN_LENGTH];
            snprintf(num_str, MAX_TOKEN_LENGTH, "%g", result.number_value);
            set_variable_value(target->data.identifier, key_str, num_str);
        } else if (result.type == RESULT_ASSOC_ARRAY) {
            printf("Error: Cannot assign an associative array to an array element\n");
        }
    } else {
        printf("Error: Invalid assignment target\n");
    }
}

int find_function(const char *name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Not found
}

void set_variable_from_eval_result(const char *name, EvalResult *result) {
    if (result->type == RESULT_NUMBER) {
        char num_str[MAX_TOKEN_LENGTH];
        snprintf(num_str, MAX_TOKEN_LENGTH, "%g", result->number_value);
        set_variable_value(name, NULL, num_str);
    } else if (result->type == RESULT_STRING) {
        set_variable_value(name, NULL, result->string_value);
    } else if (result->type == RESULT_ASSOC_ARRAY) {
        // Assign entire associative array to the variable
        set_variable_assoc_array(name, result->array_value);
    } else {
        // Unknown or unsupported type, default to "0"
        set_variable_value(name, NULL, "0");
    }
}

FunctionReturn execute_function_call(ASTNode *call_node) {

    // Check for built-in functions first
    for (int i = 0; kvstdlib_lookup_table[i].name != NULL; i++) {
        if (strcmp(kvstdlib_lookup_table[i].name, call_node->data.func_call.name) == 0) {
            if (kvstdlib_lookup_table[i].func) {
                return kvstdlib_lookup_table[i].func(call_node->data.func_call.arguments);
            }
        }
    }    

    // If not a built-in, proceed with user-defined functions
    FunctionReturn result = {0};

    int idx = find_function(call_node->data.func_call.name);
    if (idx < 0) {
        printf("Error: Undefined function '%s'\n", call_node->data.func_call.name);
        // return default 0
        result.has_return = 1;
        // result.return_is_number = 1;
        // strcpy(result.return_value, "0");
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    // Evaluate arguments
    ASTNode *param = functions[idx].parameters;
    ASTNode *arg = call_node->data.func_call.arguments;

    EvalResult args[MAX_FUNC_PARAMS];
    int argc = 0;

    while (param != NULL && arg != NULL) {
        EvalResult arg_val;
        if (!evaluate_expression(arg, &arg_val, EVAL_ARITHMETIC)) {
            printf("Error: Failed to evaluate argument\n");
            pop_scope();
            result.has_return = 1;
            // result.return_is_number = 1;
            // strcpy(result.return_value, "0");
            result.type = RESULT_NUMBER;
            result.number_value = 0;
            return result;
        }

        args[argc++] = arg_val;
        param = param->right;
        arg = arg->right;
    }


    // Push a new scope
    push_scope();

    param = functions[idx].parameters;
    int i = 0;
    while (i<argc) {

        // Assign arg_val to param->identifier
        set_variable_from_eval_result(param->data.identifier, &args[i++]);
        param = param->right;
    }

    // If fewer arguments than parameters or vice versa, decide how to handle
    // For now, missing arguments -> empty string
    while (param != NULL) {
        set_variable_value(param->data.identifier, NULL, "0");
        param = param->right;
    }

    // Execute the function body
    // Modify execute_block or a similar function to return a structure with return info
    FunctionReturn body_ret = execute_block_with_return(functions[idx].body);

    pop_scope();

    if (!body_ret.has_return) {
        // No return encountered, return 0
        body_ret.has_return = 1;
        // body_ret.return_is_number = 1;
        // strcpy(body_ret.return_value, "0");
        body_ret.type = RESULT_NUMBER;
        body_ret.number_value = 0;
    }

    return body_ret;
}

FunctionReturn execute_block_with_return(ASTNode *node) {
    FunctionReturn result = {0};
    while (node != NULL) {
        FunctionReturn stmt_result = execute_ast_with_return(node);
        if (stmt_result.has_return) {
            return stmt_result; // bubble up the return
        }
        node = node->nextblock;
    }
    return result; // no return encountered
}

void print_assoc_array(AssocArray *array) {
    printf("{");
    for (int i = 0; i < array->size; i++) {
        // Print key-value pairs
        printf("\"%s\": \"%s\"", array->pairs[i].key, array->pairs[i].value);
        if (i < array->size - 1) {
            printf(", ");
        }
    }
    printf("}\n");
}

void evaluate_and_print(ASTNode *node) {
    EvalResult result;
    if (evaluate_expression(node, &result, EVAL_PRINT)) {
        if (result.type == RESULT_STRING) {
            printf("%s\n", result.string_value);
        } else if (result.type == RESULT_NUMBER) {
            printf("%g\n", result.number_value);
        } else if (result.type == RESULT_ASSOC_ARRAY) {
            print_assoc_array(result.array_value);
        }
    }
}

void set_variable_value(const char *name, const char *key, const char *value) {
    Variable *var = NULL;
    // Check if variable exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            var = &variables[i];
            break;
        }
    }

    if (var == NULL) {
        // Create new variable
        if (variable_count >= MAX_VARIABLES) {
            printf("Error: Maximum number of variables reached\n");
            return;
        }
        strcpy(variables[variable_count].name, name);
        init_assoc_array(&variables[variable_count].array);
        var = &variables[variable_count];
        variable_count++;
    }

    if (key == NULL) {
        // Assign to default key
        set_assoc_array_value(&var->array, "", value);
    } else {
        // Assign to specified key
        set_assoc_array_value(&var->array, key, value);
    }
}

Variable* get_variable(const char *name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }
    return NULL; // Variable not found
}

char* get_variable_value(const char *name) {
    // Lookup variable
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            // Get value from default key
            char *value = get_assoc_array_value(&variables[i].array, "");
            if (value != NULL) {
                return value;
            } else {
                // If no default key, return a representation of the array
                return "[Associative Array]";
            }
        }
    }
    return NULL; // Variable not found
}


void free_ast(ASTNode *node) {
    if (node == NULL) return;
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->nextblock);
    free(node);
}

int starts_with_keyword(const char *line, const char *keyword) {
    int len = strlen(keyword);
    // Skip leading whitespace
    while (*line && isspace(*line)) {
        line++;
    }
    return strncmp(line, keyword, len) == 0 && (line[len] == '\0' || isspace(line[len]));
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        // Run script file
        const char *filename = argv[1];
        FILE *file = fopen(filename, "r");
        if (!file) {
            printf("Error: Could not open file '%s'\n", filename);
            return 1;
        }

        char line[MAX_LINE_LENGTH];
        char buffer[MAX_LINE_LENGTH * 1000]; // Adjust size as needed
        buffer[0] = '\0';

        while (fgets(line, sizeof(line), file)) {
            strcat(buffer, line);
        }
        fclose(file);

        // Tokenize, parse, and execute the buffer
        Token tokens[MAX_TOKENS * 100]; // Adjust size as needed
        int token_count = 0;
        tokenize_line(buffer, tokens, &token_count);
        parse_and_execute(tokens, token_count);
    } else {
        char line[MAX_LINE_LENGTH];
        char buffer[MAX_LINE_LENGTH * 100]; // Adjust size as needed
        buffer[0] = '\0'; // Initialize buffer
        int in_block = 0; // Flag to indicate if we're inside a block

        printf("Welcome to keyva-lang REPL\n");
        while (1) {
            // Display prompt based on block depth
            if (in_block > 0) {
                printf("... "); // Indicate continuation
            } else {
                printf("> ");
            }
            if (fgets(line, sizeof(line), stdin) == NULL) break;

            // Remove trailing newline character
            line[strcspn(line, "\n")] = '\0';

            // Check for exit command
            if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
                break;
            }

            // Append line to buffer
            strcat(buffer, line);
            strcat(buffer, "\n"); // Keep newlines to maintain line numbers

            // Check for block-opening keywords
            if (starts_with_keyword(line, "if")) {
                in_block++;
            }

            // Check for block-opening keywords
            if (starts_with_keyword(line, "for")) {
                in_block++;
            }

            // Check for block-opening keywords
            if (starts_with_keyword(line, "def")) {
                in_block++;
            }

            // Check for block-closing keyword
            if (starts_with_keyword(line, "end")) {
                in_block--;
                if (in_block < 0) {
                    printf("Error: Unmatched 'end' detected\n");
                    buffer[0] = '\0';
                    in_block = 0;
                    continue;
                }
            }

            // If not inside a block, process the buffer
            if (in_block == 0) {
                // Tokenize, parse, and execute the buffer
                Token tokens[MAX_TOKENS];
                int token_count = 0;
                tokenize_line(buffer, tokens, &token_count);
                parse_and_execute(tokens, token_count);

                // Clear the buffer
                buffer[0] = '\0';
            }
        }

        return 0;
    }

    return 0;
}


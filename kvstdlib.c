/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (C) 2024 Gary Sims
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// #define NDEBUG 1
#define DEBUG 1
#include "debug_print.h"

#include "kvlang_internals.h"

#include "kvstdlib.h"

FunctionReturn kvstdlib_len(ASTNode *arg) {
    FunctionReturn result = {0};

    // len requires exactly one argument
    if (arg == NULL || arg->right != NULL) {
        printf("Error: len() requires exactly one argument\n");
        result.has_return = 1;
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    EvalResult arg_val;
    if (!evaluate_expression(arg, &arg_val, EVAL_PRINT)) {
        printf("Error: Failed to evaluate argument in len()\n");
        result.has_return = 1;
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    int length = 0;
    switch (arg_val.type) {
        case RESULT_ASSOC_ARRAY:
            // Just return array size
            length = arg_val.array_value->size;
            break;
        case RESULT_NUMBER:
        case RESULT_STRING:
            // Treat numbers and strings as arrays of length 1
            length = 1;
            break;
        default:
            // Unknown type, return 0
            length = 0;
            break;
    }

    result.has_return = 1;
    result.type = RESULT_NUMBER;
    result.number_value = length;
    return result;
}

FunctionReturn kvstdlib_key(ASTNode *arg) {
    FunctionReturn result = {0};
    result.has_return = 1;

    // Requires exactly one argument
    if (arg == NULL || arg->right != NULL) {
        printf("Error: key() requires exactly one argument\n");
        result.type = RESULT_STRING;
        strcpy(result.string_value, "");
        return result;
    }

    if(arg->type == AST_IDENTIFIER) {
        Variable *var = get_variable(arg->data.identifier);
        if (var != NULL) {
            result.type = RESULT_STRING;
            strcpy(result.string_value, var->array.pairs[0].key);
            return result;
        }
    } else if(arg->type == AST_ARRAY_ACCESS) {
        EvalResult key_result;
        if (!evaluate_expression(arg->left, &key_result, EVAL_ARITHMETIC)) {
            printf("Error: Failed to evaluate array index\n");
            result.type = RESULT_STRING;
            strcpy(result.string_value, "");
            return result;
        }
        if (key_result.type != RESULT_STRING && key_result.type != RESULT_NUMBER) {
            printf("Error: Array index must be a string or number\n");
            result.type = RESULT_STRING;
            strcpy(result.string_value, "");
            return result;
        }

        // Convert numeric index to string if necessary
        if (key_result.type == RESULT_NUMBER) {
            snprintf(result.string_value, MAX_TOKEN_LENGTH, "%g", key_result.number_value);
        } else {
            strcpy(result.string_value, key_result.string_value);
        }        
        result.type = RESULT_STRING;
        return result;
    }

    // Default
    result.has_return = 1;
    result.type = RESULT_STRING;
    strcpy(result.string_value, "");
    return result;    
}

FunctionReturn kvstdlib_mod(ASTNode *arg) {
    FunctionReturn result = {0};
    result.has_return = 1;

    // Requires exactly one argument
    if (arg == NULL || arg->right == NULL) {
        printf("Error: mod() requires exactly two argument\n");
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    EvalResult arg_val1;
    if (!evaluate_expression(arg, &arg_val1, EVAL_ARITHMETIC)) {
        printf("Error: Failed to evaluate 1st argument in mod()\n");
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    EvalResult arg_val2;
    if (!evaluate_expression(arg->right, &arg_val2, EVAL_ARITHMETIC)) {
        printf("Error: Failed to evaluate 2nd argument in mod()\n");
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    if(arg_val1.type!=RESULT_NUMBER || arg_val2.type!=RESULT_NUMBER) {
        result.type = RESULT_NUMBER;
        result.number_value = 0;
        return result;
    }

    result.type = RESULT_NUMBER;
    result.number_value = ((int) arg_val1.number_value) % ((int) arg_val2.number_value);
    return result;
}

FunctionReturn kvstdlib_bar(ASTNode *arg) {
    FunctionReturn result = {0};
    result.has_return = 1;
    return result;
}

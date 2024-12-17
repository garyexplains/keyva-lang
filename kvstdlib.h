
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  Copyright (C) 2024 Gary Sims
 *
 */

#ifndef KVSTDLIB_H
#define KVSTDLIB_H

#include "kvlang_internals.h"

/* Define a standard lib function type */
typedef FunctionReturn (*kvstdlib_func_t)(ASTNode *arg);

/* Forward declarations of standard lib functions */
FunctionReturn kvstdlib_len(ASTNode *arg);
FunctionReturn kvstdlib_key(ASTNode *arg);
FunctionReturn kvstdlib_mod(ASTNode *arg);
FunctionReturn kvstdlib_bar(ASTNode *arg);

/* Structure to associate a string with its function */
typedef struct {
    const char *name;
    kvstdlib_func_t func;
} kvstdlib_lookup_entry_t;

/* Array of name/callback pairs */
static const kvstdlib_lookup_entry_t kvstdlib_lookup_table[] = {
    { "len", kvstdlib_len },
    { "key", kvstdlib_key },
    { "mod", kvstdlib_mod },
    { "bar", kvstdlib_bar },
    { NULL, NULL } /* Sentinel to mark the end of the array */
};


#endif /* KVSTDLIB_H */

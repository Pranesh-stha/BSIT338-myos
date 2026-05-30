#ifndef EXPR_H
#define EXPR_H

typedef enum {
    EXPR_OK = 0,
    EXPR_SYNTAX,
    EXPR_DIV_ZERO,
} ExprError;

// Parses a single integer arithmetic expression. Supports + - * / %,
// parentheses, unary +/-. Whitespace ignored. Returns EXPR_OK and
// writes the value to *result on success.
ExprError Expr_Eval(const char* input, int* result);

#endif

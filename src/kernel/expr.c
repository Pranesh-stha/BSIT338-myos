#include "expr.h"

// Recursive-descent integer expression parser.
//
// Grammar:
//   expr   = term   (('+' | '-') term)*
//   term   = factor (('*' | '/' | '%') factor)*
//   factor = number | '(' expr ')' | '-' factor | '+' factor
//
// Operator precedence is encoded by the grammar levels (expr below term).

static const char* g_p;
static ExprError   g_Error;

static void skip_ws(void)
{
    while (*g_p == ' ' || *g_p == '\t') g_p++;
}

static int is_digit(char c) { return c >= '0' && c <= '9'; }

static int parse_number(void)
{
    int n = 0;
    while (is_digit(*g_p))
    {
        n = n * 10 + (*g_p - '0');
        g_p++;
    }
    return n;
}

static int parse_expr(void);

static int parse_factor(void)
{
    skip_ws();
    if (*g_p == '(')
    {
        g_p++;
        int v = parse_expr();
        skip_ws();
        if (*g_p != ')')
        {
            if (g_Error == EXPR_OK) g_Error = EXPR_SYNTAX;
            return 0;
        }
        g_p++;
        return v;
    }
    if (*g_p == '-') { g_p++; return -parse_factor(); }
    if (*g_p == '+') { g_p++; return  parse_factor(); }
    if (is_digit(*g_p))
        return parse_number();

    if (g_Error == EXPR_OK) g_Error = EXPR_SYNTAX;
    return 0;
}

static int parse_term(void)
{
    int v = parse_factor();
    for (;;)
    {
        skip_ws();
        if      (*g_p == '*') { g_p++; v = v * parse_factor(); }
        else if (*g_p == '/')
        {
            g_p++;
            int rhs = parse_factor();
            if (rhs == 0)
            {
                if (g_Error == EXPR_OK) g_Error = EXPR_DIV_ZERO;
                return 0;
            }
            v = v / rhs;
        }
        else if (*g_p == '%')
        {
            g_p++;
            int rhs = parse_factor();
            if (rhs == 0)
            {
                if (g_Error == EXPR_OK) g_Error = EXPR_DIV_ZERO;
                return 0;
            }
            v = v % rhs;
        }
        else break;
    }
    return v;
}

static int parse_expr(void)
{
    int v = parse_term();
    for (;;)
    {
        skip_ws();
        if      (*g_p == '+') { g_p++; v = v + parse_term(); }
        else if (*g_p == '-') { g_p++; v = v - parse_term(); }
        else break;
    }
    return v;
}

ExprError Expr_Eval(const char* input, int* result)
{
    g_p = input;
    g_Error = EXPR_OK;

    int v = parse_expr();

    if (g_Error != EXPR_OK) return g_Error;

    skip_ws();
    if (*g_p != 0) return EXPR_SYNTAX;   // trailing garbage

    *result = v;
    return EXPR_OK;
}

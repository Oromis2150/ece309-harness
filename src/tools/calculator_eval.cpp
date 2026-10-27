// src/tools/calculator_eval.cpp — YOU IMPLEMENT (spec §4.6).
//
// calc_evaluate: evaluate a well-formed token list with two stacks
// (one for numbers, one for operators and '('). See spec §4.6 and the
// worked trace in Appendix C. std::stack or std::vector are both fine.

#include "tools/calculator.h"

namespace {
    using K = CalcToken::Kind;
    int precedence(K k) { return k == K::Times || k == K::Divide ? 2 : 1; }
    bool apply(std::vector<double>& values, std::vector<K>& ops, std::string* error) {
        if (values.size() < 2 || ops.empty() || ops.back() == K::LParen) {
            *error = "invalid expression"; return false;
        }
        K op = ops.back(); ops.pop_back();
        double right = values.back(); values.pop_back();
        double left = values.back(); values.pop_back();
        if (op == K::Divide && right == 0.0) {*error = "division by zero"; return false;}
        switch(op) {
            case K::Plus: values.push_back(left + right); break;
            case K::Minus: values.push_back(left - right); break;
            case K::Times: values.push_back(left * right); break;
            case K::Divide: values.push_back(left / right); break;
            default: *error = "invalid expression"; return false;
        }
        return true;
    }
}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
                   std::string* error) {
    std::vector<double> values;
    std::vector<K> ops;
    bool expect_value = true;
    for (const auto& token : tokens) {
        K k = token.kind;
        if (k == K::Number) {
            if (!expect_value) { *error = "invalid expression"; return false;}
            values.push_back(token.value); expect_value = false;
        } else if (k == K::LParen) {
            if (!expect_value) { *error = "invalid expression"; return false;}
            ops.push_back(k);
        } else if (k == K::RParen) {
            if (expect_value) { *error = "invalid expression"; return false;}
            while (!ops.empty() && ops.back() != K::LParen)
                if (!apply(values, ops, error)) return false;
            if (ops.empty()) { *error = "invalid expression"; return false;}
            ops.pop_back(); expect_value = false;
        } else if (k == K::Plus || k == K::Minus || k == K::Times || k == K::Divide) {
            if (expect_value) { *error = "invalid expression"; return false;}
            while (!ops.empty() && ops.back() != K::LParen && precedence(ops.back()) >= precedence(k))
                if (!apply(values, ops, error)) return false;
            ops.push_back(k); expect_value = true;
        } else { *error = "invalid expression"; return false; }
    }
    if (expect_value) { *error = "invalid expression"; return false;}
    while (!ops.empty()) if (!apply(values, ops, error)) return false;
    if (values.size() != 1) { *error = "invalid expression"; return false; }
    *out = values.back(); return true;
}
    

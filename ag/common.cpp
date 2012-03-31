#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>

#include "common.hpp"
#include "gram.tab.hpp"

#define INDENT (2)

SymbolTable syms;

using std::stringstream;
using std::endl;

string NumberExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "NUM: " << m_val << endl;
    return s.str();
}

string SymbolExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "SYM: " << syms.get(m_sym) << endl;
    return s.str();
}

vector<sym_t> SymbolExprAST::collectDefinedSymbols() {
    vector<sym_t> v;
    v.push_back(m_sym);
    return v;
}

int SymbolExprAST::checkSymbols(Scope *scope) {
    if (!scope->contains(m_sym)) {
        fprintf(stderr, "undefined reference to '%s'\n", syms.get(m_sym).c_str());
        return 1;
    }
    return 0;
}

string ListExprAST::toString(int level) const {
    stringstream s;
    for (unsigned int i = 0; i < m_exprs.size(); i++) {
        s << m_exprs[i]->toString(level);
    }
    return s.str();
}

ListExprAST::~ListExprAST() {
    for (unsigned int i = 0; i < m_exprs.size(); i++) {
        delete m_exprs[i];
    }
}

ListExprAST *ListExprAST::push_back(ListExprAST *l, ExprAST *e) {
    if (l == NULL) {
        l = new ListExprAST();
    }
    l->m_exprs.push_back(e);
    return l;
}

vector<sym_t> ListExprAST::collectDefinedSymbols() {
    vector<sym_t> v;
    for (unsigned int i = 0; i < m_exprs.size(); i++) {
        vector<sym_t> w = m_exprs[i]->collectDefinedSymbols();
        v.insert(v.end(), w.begin(), w.end());
    }
    return v;
}

int ListExprAST::checkSymbols(Scope *scope) {
    int j = 0;
    for (unsigned int i = 0; i < m_exprs.size(); i++) {
        j += m_exprs[i]->checkSymbols(scope);
    }
    return j;
}

string SymListExprAST::toString(int level) const {
    stringstream s;
    for (unsigned int i = 0; i < m_syms.size(); i++) {
        s << string(level * INDENT, ' ') << "SYM: " << syms.get(m_syms[i]) << endl;
    }
    return s.str();
}

SymListExprAST *SymListExprAST::push_back(SymListExprAST *l, sym_t e) {
    if (l == NULL) {
        l = new SymListExprAST;
        l->m_syms = vector<sym_t>();
    }
    l->m_syms.push_back(e);
    return l;
}

vector<sym_t> SymListExprAST::collectDefinedSymbols() {
    /* Either parameter lists in a function definition or
     * statement labels. */
    return m_syms;
}

string FunctionExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "FUN: " << syms.get(m_name);
    if (m_scope != NULL) {
        s << "; " << m_scope->toString();
    } else {
        s << endl;
    }
    s << "PARS:" << endl;
    if (m_pars != NULL) {
        s << m_pars->toString(level + 1);
    }
    s << "STATS:" << endl;
    if (m_stats != NULL) {
        s << m_stats->toString(level + 1);
    }
    return s.str();
}

FunctionExprAST::~FunctionExprAST() {
    delete m_pars;
    delete m_stats;
    delete m_scope;
}

vector<sym_t> FunctionExprAST::collectDefinedSymbols() {
    m_scope = new Scope;
    if (m_pars != NULL) {
        m_scope->insertAll(m_pars->collectDefinedSymbols());
    }
    if (m_stats != NULL) {
        m_scope->insertAll(m_stats->collectDefinedSymbols());
    }
    return vector<sym_t>();
}

int FunctionExprAST::checkSymbols(Scope *scope) {
    assert(scope == NULL);
    assert(m_scope != NULL);

    if (m_stats != NULL) {
        return m_stats->checkSymbols(m_scope);
    } else {
        return 0;
    }
}

string StatementExprAST::toString(int level) const {
    stringstream s;
    if (m_labels != NULL) {
        s << m_labels->toString(level);
    }
    s << m_stat->toString(level);
    return s.str();
}

StatementExprAST::~StatementExprAST() {
    delete m_labels;
    delete m_stat;
}

vector<sym_t> StatementExprAST::collectDefinedSymbols() {
    vector<sym_t> v = m_stat->collectDefinedSymbols();
    if (m_labels != NULL) {
        vector<sym_t> w = m_labels->collectDefinedSymbols();
        v.insert(v.end(), w.begin(), w.end());
    }
    return v;
}

string CallExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "CALL: " << syms.get(m_callee) << endl;
    s << m_args->toString(level + 1);
    return s.str();
}

CallExprAST::~CallExprAST() {
    delete m_args;
}

static const char *opstr(int op) {
    switch (op) {
    case DEREF: return "DEREF";
    case UNARYMINUS: return "UNARYMINUS";
    case RETURN: return "RETURN";
    case OPLESSEQ: return "OPLESSEQ";
    case GOTO: return "GOTO";
    case IF: return "IF";
    case NOT: return "NOT";
    case AND: return "AND";
    case VAR: return "VAR";
    case '=': return "=";
    case '*': return "*";
    case '-': return "-";
    case '+': return "+";
    case '#': return "#";
    default: return "???";
    }
}

string BinaryExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << opstr(m_op);
    if (m_scope != NULL) {
        s << "; " << m_scope->toString();
    } else {
        s << endl;
    }
    s << m_lhs->toString(level + 1);
    s << m_rhs->toString(level + 1);
    return s.str();
}

BinaryExprAST::~BinaryExprAST() {
    delete m_lhs;
    delete m_rhs;
}

vector<sym_t> BinaryExprAST::collectDefinedSymbols() {
    switch (m_op) {
    case IF:
        m_scope = new Scope;
        m_scope->insertAll(m_rhs->collectDefinedSymbols());
        break;
    case VAR: return m_lhs->collectDefinedSymbols();
    default: break;
    }

    return vector<sym_t>();
}

int BinaryExprAST::checkSymbols(Scope *scope) {
    assert(scope != NULL);
    
    if (m_op == IF) {
        assert(m_scope != NULL);
        m_scope->merge(scope);
        scope = m_scope;
    }

    int j = 0;
    j += m_lhs->checkSymbols(scope);
    j += m_rhs->checkSymbols(scope);
    return j;
}

string UnaryExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << opstr(m_op) << endl;
    s << m_arg->toString(level + 1);
    return s.str();
}

UnaryExprAST::~UnaryExprAST() {
    delete m_arg;
}

sym_t SymbolTable::insert(string s) {
    unsigned int i;
    for (i = 0; i < m_symbols.size(); i++) {
        if (s == m_symbols[i]) {
            return i;
        }
    }
    m_symbols.push_back(s);
    return i;
}

string SymbolTable::get(sym_t i) const {
    assert(i < (int)m_symbols.size());
    return m_symbols[i];
}

string SymbolTable::toString() const {
    stringstream s;
    s << "Symbol table contents:" << endl;
    for (unsigned int i = 0; i < m_symbols.size(); i++) {
        s << i << ": " << m_symbols[i] << endl;
    }
    return s.str();
}

void Scope::merge(const Scope *that) {
    insertAll(that->m_symbols);
}

void Scope::insert(sym_t s) {
    if (contains(s)) {
        fprintf(stderr, "Redefinition of symbol '%s'\n", syms.get(s).c_str());
        exit(ERR_SCOPE);
    }
    m_symbols.push_back(s);
}

void Scope::insertAll(vector<sym_t> v) {
    for (unsigned int i = 0; i < v.size(); i++) {
        insert(v[i]);
    }
}

int Scope::contains(sym_t s) const {
    for (unsigned int i = 0; i < m_symbols.size(); i++) {
        if (s == m_symbols[i]) {
            return 1;
        }
    }
    return 0;
}

string Scope::toString() const {
    stringstream s;
    s << "Current scope: ";
    for (unsigned int i = 0; i < m_symbols.size(); i++) {
        s << syms.get(m_symbols[i]) << ",";
    }
    s << endl;
    return s.str();
}

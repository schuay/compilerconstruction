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

vector<Symbol> SymbolExprAST::collectDefinedSymbols() {
    vector<Symbol> v;
    v.push_back(Symbol(m_sym, Var));
    return v;
}

int SymbolExprAST::checkSymbols(Scope *scope) {
    if (!scope->contains(m_sym)) {
        fprintf(stderr, "undefined reference to '%s'\n", syms.get(m_sym).c_str());
        return 1;
    }
    return 0;
}

FunctionExprAST::FunctionExprAST(sym_t name, SymList *pars, ExprList *stats)
    : ExprAST(), m_name(name) {
    /* TODO: pars are reversed */
    if (pars) {
        m_pars = pars->get();
        delete pars;
    }
    if (stats) {
        m_stats = stats->get();
        delete stats;
    }
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
    for (unsigned int i = 0; i < m_pars.size(); i++) {
        s << syms.get(m_pars[i]);
    }
    s << "STATS:" << endl;
    for (unsigned int i = 0; i < m_stats.size(); i++) {
        s << m_stats[i]->toString(level + 1);
    }
    return s.str();
}

FunctionExprAST::~FunctionExprAST() {
    for (unsigned int i = 0; i < m_stats.size(); i++) {
        delete m_stats[i];
    }
    delete m_scope;
}

vector<Symbol> FunctionExprAST::collectDefinedSymbols() {
    m_scope = new Scope;
    m_scope->insertAll(m_pars);
    for (unsigned int i = 0; i < m_stats.size(); i++) {
        m_scope->insertAll(m_stats[i]->collectDefinedSymbols());
    }
    return vector<Symbol>();
}

int FunctionExprAST::checkSymbols(Scope *scope) {
    assert(scope == NULL);
    assert(m_scope != NULL);

    int j = 0;
    for (unsigned int i = 0; i < m_stats.size(); i++) {
        j += m_stats[i]->checkSymbols(m_scope);
    }

    return j;
}

string StatementExprAST::toString(int level) const {
    stringstream s;
    for (unsigned int i = 0; i < m_labels.size(); i++) {
        s << syms.get(m_labels[i]);
    }
    s << m_stat->toString(level);
    return s.str();
}

StatementExprAST::~StatementExprAST() {
    delete m_stat;
}

vector<Symbol> StatementExprAST::collectDefinedSymbols() {
    vector<Symbol> v = m_stat->collectDefinedSymbols();
    for (unsigned int i = 0; i < m_labels.size(); i++) {
        v.push_back(Symbol(m_labels[i], Label));
    }
    return v;
}

CallExprAST::CallExprAST(sym_t callee, ExprList *args)
    : ExprAST(), m_callee(callee)
{
    if (args != NULL) {
        m_args = args->get();        
        delete args;
    }
}

string CallExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "CALL: " << syms.get(m_callee) << endl;
    for (unsigned int i = 0; i < m_args.size(); i++) {
        s << m_args[i]->toString(level + 1);
    }
    return s.str();
}

CallExprAST::~CallExprAST() {
    for (unsigned int i = 0; i < m_args.size(); i++) {
        delete m_args[i];
    }
}

int CallExprAST::checkSymbols(Scope *scope) {
    int j = 0;
    for (unsigned int i = 0; i < m_args.size(); i++) {
        j += m_args[i]->checkSymbols(scope);
    }
    return j;
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

IfExprAST::IfExprAST(ExprAST *cond, ExprList *then)
    : ExprAST(), m_cond(cond)
{
    if (then != NULL) {
        m_then = then->get();
        delete then;
    }
}

string IfExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "IF; " << m_scope->toString();
    s << m_cond->toString(level + 1);
    for (unsigned int i = 0; i < m_then.size(); i++) {
        s << m_then[i]->toString(level + 1);
    }
    return s.str();
}

IfExprAST::~IfExprAST() {
    delete m_cond;
    for (unsigned int i = 0; i < m_then.size(); i++) {
        delete m_then[i];
    }
}

vector<Symbol> IfExprAST::collectDefinedSymbols() {
    m_scope = new Scope;
    vector<Symbol> syms;
    for (unsigned int i = 0; i < m_then.size(); i++) {
        vector<Symbol> tsyms = m_then[i]->collectDefinedSymbols();
        syms.insert(syms.begin(), tsyms.begin(), tsyms.end());
    }
    for (vector<Symbol>::iterator it = syms.begin(); it != syms.end(); ) {
        Symbol s = *it;
        if (s.type == Label) {
            it++;
            continue;
        }
        m_scope->insert(s.sym);
        it = syms.erase(it);
    }

    return syms;
}

int IfExprAST::checkSymbols(Scope *scope) {
    assert(scope != NULL);

    /* lhs is always in the parent scope.  */

    int j = 0;
    j += m_cond->checkSymbols(scope);

    /* rhs starts a new (merged) scope in IF statements. */

    assert(m_scope != NULL);
    m_scope->merge(scope);
    scope = m_scope;

    for (unsigned int i = 0; i < m_then.size(); i++) {
        j += m_then[i]->checkSymbols(scope);
    }
    return j;
}

string BinaryExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << opstr(m_op) << endl;
    s << m_lhs->toString(level + 1);
    s << m_rhs->toString(level + 1);
    return s.str();
}

BinaryExprAST::~BinaryExprAST() {
    delete m_lhs;
    delete m_rhs;
}

vector<Symbol> BinaryExprAST::collectDefinedSymbols() {
    switch (m_op) {
    case VAR: return m_lhs->collectDefinedSymbols();
    default: break;
    }
    return vector<Symbol>();
}

int BinaryExprAST::checkSymbols(Scope *scope) {
    assert(scope != NULL);

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

void Scope::insertAll(vector<Symbol> v) {
    for (unsigned int i = 0; i < v.size(); i++) {
        insert(v[i].sym);
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

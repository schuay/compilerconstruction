#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <map>
#include <sstream>
#include <llvm/Constants.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/IRBuilder.h>

#include "common.hpp"
#include "gram.tab.hpp"

#define INDENT (2)

SymbolTable syms;

using std::stringstream;
using std::endl;
using std::map;

static Module *theModule;
static IRBuilder<> builder(getGlobalContext());
static map<int, Value*> namedValues;

Value *errorV(const char *str) { fprintf(stderr, "Error: %s\n", str); return 0; }

string NumberExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "NUM: " << m_val << endl;
    return s.str();
}

Value *NumberExprAST::codegen() {
    /* 64 bits, signed */
    return ConstantInt::get(getGlobalContext(), APInt(64, m_val, true));
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

Value *SymbolExprAST::codegen() {
    /* TODO: this also needs to work for labels. */
    Value *v = namedValues[m_sym];
    return v ? v : errorV("Unknown variable name");
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

vector<Symbol> ListExprAST::collectDefinedSymbols() {
    vector<Symbol> v;
    for (unsigned int i = 0; i < m_exprs.size(); i++) {
        vector<Symbol> w = m_exprs[i]->collectDefinedSymbols();
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

Value *ListExprAST::codegen() {
    Value *v;
    for (unsigned int i = 0; i < m_exprs.size(); i++) {
        v = m_exprs[i]->codegen();
        if (v == 0) {
            break;
        }
    }

    return v;
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

vector<Symbol> SymListExprAST::collectDefinedSymbols() {
    /* Either parameter lists in a function definition or
     * statement labels. We don't know which one it is, so
     * make it a Var here and adjust later if necessary. */
    vector<Symbol> v;
    for (unsigned int i = 0; i < m_syms.size(); i++) {
        v.push_back(Symbol(m_syms[i], Var));
    }
    return v;
}

Value *SymListExprAST::codegen() {
    return 0; /* TODO */
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

vector<Symbol> FunctionExprAST::collectDefinedSymbols() {
    m_scope = new Scope;
    if (m_pars != NULL) {
        m_scope->insertAll(m_pars->collectDefinedSymbols());
    }
    if (m_stats != NULL) {
        m_scope->insertAll(m_stats->collectDefinedSymbols());
    }
    return vector<Symbol>();
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

Value *FunctionExprAST::codegen() {
    return 0; /* TODO */
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

vector<Symbol> StatementExprAST::collectDefinedSymbols() {
    vector<Symbol> v = m_stat->collectDefinedSymbols();
    if (m_labels != NULL) {
        vector<Symbol> w = m_labels->collectDefinedSymbols();
        for (unsigned int i = 0; i < w.size(); i++) {
            Symbol s = w[i];
            s.type = Label;
            v.push_back(s);
        }
    }
    return v;
}

Value *StatementExprAST::codegen() {
    return 0; /* TODO */
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

Value *CallExprAST::codegen() {
    return 0; /* TODO */
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

string IfExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "IF; " << m_scope->toString();
    s << m_cond->toString(level + 1);
    s << m_then->toString(level + 1);
    return s.str();
}

IfExprAST::~IfExprAST() {
    delete m_cond;
    delete m_then;
}

vector<Symbol> IfExprAST::collectDefinedSymbols() {
    m_scope = new Scope;
    vector<Symbol> syms = m_then->collectDefinedSymbols();
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

    j += m_then->checkSymbols(scope);
    return j;
}

Value *IfExprAST::codegen() {
    return 0; /* TODO */
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

Value *BinaryExprAST::codegen() {
    Value *l = m_lhs->codegen();
    Value *r = m_rhs->codegen();
    if (l == 0 || r == 0) {
        return 0;
    }

    switch (m_op) {
    case VAR:
    case '=': return errorV("{VAR, =} not yet implemented"); /* TODO */
    case '*': return builder.CreateMul(l, r, "multmp");
    case '+': return builder.CreateAdd(l, r, "addtmp");
    case AND: return builder.CreateAnd(l, r, "andtmp");
    case OPLESSEQ: return builder.CreateICmpSLE(l, r, "cmptmp");
    case '#':return builder.CreateICmpNE(l, r);
    default: return errorV("Unknown binary operator.");
    }
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

Value *UnaryExprAST::codegen() {
    Value *v = m_arg->codegen();
    if (v == 0) {
        return 0;
    }

    switch (m_op) {
    case NOT: return builder.CreateNot(v, "nottmp");
    case UNARYMINUS: return builder.CreateNeg(v, "negtmp");
    case RETURN: builder.CreateRet(v);
    case DEREF:
    case GOTO: return errorV("{DEREF,GOTO} not yet implemented"); /* TODO */
    default: return errorV("Unknown unary operator.");
    }
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

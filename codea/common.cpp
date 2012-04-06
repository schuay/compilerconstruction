#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>

#include "common.hpp"
#include "gram.tab.hpp"

#define INDENT (2)

SymbolTable syms;

using std::stringstream;
using std::endl;
using std::map;

/* Globals used during code generation. */
Module *theModule = new Module("mainmodule", getGlobalContext());
static IRBuilder<> builder(getGlobalContext());
static map<int, AllocaInst*> namedValues;

void printAsm() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    /* Create outstream. */
    raw_fd_ostream rostr(fileno(stdout), false);
    formatted_raw_ostream frostr(rostr);

    /* Retrieve information about target machine. */
    Triple trp("x86_64-linux-gnu");
    theModule->setTargetTriple(trp.getTriple());

    string err;
    const Target *trg = TargetRegistry::lookupTarget(trp.getTriple(), err);
    if (trg == NULL) {
        std::cerr << err << endl;
        exit(ERR_SCOPE);
    }

    TargetMachine *tgm = trg->createTargetMachine(trp.getTriple(), "", "");

    /* Create and configure pass manager. */
    PassManager pm;
    pm.add(new TargetData(theModule));

    /* Override default to generate verbose assembly. */
    tgm->setAsmVerbosityDefault(true);

    /* Optimizations. */
    pm.add(createBasicAliasAnalysisPass());
    pm.add(createInstructionCombiningPass());
    pm.add(createReassociatePass());
    pm.add(createGVNPass());
    pm.add(createCFGSimplificationPass());
    pm.add(createPromoteMemoryToRegisterPass());

    /* Add pass to print asm. */
    tgm->addPassesToEmitFile(pm, frostr, TargetMachine::CGFT_AssemblyFile,
                            CodeGenOpt::Default, false);

    /* Run passes. */
    pm.run(*theModule);

    delete tgm;
}

static AllocaInst *createEntryBlockAlloca(Function *f, sym_t s) {
    IRBuilder<> b(&f->getEntryBlock(),f->getEntryBlock().begin());
    return b.CreateAlloca(Type::getInt64Ty(getGlobalContext()), 0, syms.get(s).c_str());
}

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
    v.push_back(Symbol(m_sym, m_type));
    return v;
}

int SymbolExprAST::checkSymbols(Scope *scope) {
    if (!scope->contains(m_sym, m_type)) {
        fprintf(stderr, "undefined reference to '%s'\n", syms.get(m_sym).c_str());
        return 1;
    }
    return 0;
}

Value *SymbolExprAST::codegen() {
    /* TODO: this also needs to work for labels. */
    Value *v = namedValues[m_sym];
    if (v == 0) {
        return errorV("Unknown variable name");
    }
    return builder.CreateLoad(v, m_sym);
}

string AddrExprAST::toString(int level) const {
    stringstream s;
    s << string(level * INDENT, ' ') << "SYMADDR: " << syms.get(m_sym) << endl;
    return s.str();
}

vector<Symbol> AddrExprAST::collectDefinedSymbols() {
    vector<Symbol> v;
    v.push_back(Symbol(m_sym, m_type));
    return v;
}

int AddrExprAST::checkSymbols(Scope *scope) {
    if (!scope->contains(m_sym, m_type)) {
        fprintf(stderr, "undefined reference to '%s'\n", syms.get(m_sym).c_str());
        return 1;
    }
    return 0;
}

Value *AddrExprAST::codegen() {
    AllocaInst *v = namedValues[m_sym];
    return (v != 0 ? v : errorV("Unknown variable name"));
}

FunctionExprAST::FunctionExprAST(sym_t name, SymList *pars, ExprList *stats)
    : ExprAST(), m_name(name) {
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
    m_scope->insertAll(m_pars, Var);
    for (unsigned int i = 0; i < m_stats.size(); i++) {
        m_scope->insertAll(m_stats[i]->collectDefinedSymbols());
    }
    return vector<Symbol>();
}

int FunctionExprAST::checkSymbols(Scope * /* scope UNUSED */) {
    assert(scope == NULL);
    assert(m_scope != NULL);

    int j = 0;
    for (unsigned int i = 0; i < m_stats.size(); i++) {
        j += m_stats[i]->checkSymbols(m_scope);
    }

    return j;
}

Value *FunctionExprAST::codegen() {
    namedValues.clear();

    vector<Type *> ints(m_pars.size(),
                        Type::getInt64Ty(getGlobalContext()));
    FunctionType *ft = FunctionType::get(Type::getInt64Ty(getGlobalContext()),
                                         ints, false);
    Function *f = Function::Create(ft, Function::ExternalLinkage,
                                   syms.get(m_name), theModule);

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", f);
    builder.SetInsertPoint(bb);


    /* Create all local vars on the stack and store them in namedValues. */
    const vector<sym_t> variables = m_scope->variables();
    for (unsigned int i = 0; i < variables.size(); i++) {
        AllocaInst *alloca = createEntryBlockAlloca(f, variables[i]);
        namedValues[variables[i]] = alloca;
    }

    /* Name args and store their values. */
    unsigned int i = 0;
    for (Function::arg_iterator ai = f->arg_begin(); i != m_pars.size();
         ++ai, ++i) {
        ai->setName(syms.get(m_pars[i]));
        builder.CreateStore(ai, namedValues[m_pars[i]]);
    }

    if (m_stats.size() > 0) {
        for (unsigned int i = 0; i < m_stats.size(); i++) {
            Value *retVal = m_stats[i]->codegen();
            if (retVal == 0) {
                f->eraseFromParent();
                return 0;
            }
        }
    } else {
        builder.CreateRet(ConstantInt::get(getGlobalContext(), APInt(64, 0, true)));
    }

    verifyFunction(*f);

    return f;
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

Value *StatementExprAST::codegen() {
    /* TODO labels*/
    return m_stat->codegen();
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
        m_scope->insert(s);
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
    case '=': return builder.CreateStore(r, l);    /* TODO: this needs to work for memory locations, not just VARs */
    case '*': return builder.CreateMul(l, r, "multmp");
    case '+': return builder.CreateAdd(l, r, "addtmp");
    case AND: return builder.CreateAnd(l, r, "andtmp");
    case OPLESSEQ:
        l = builder.CreateICmpSLE(l, r, "cmptmp");
        return builder.CreateZExt(l, Type::getInt64Ty(getGlobalContext()), "csttmp");
    case '#':
        l = builder.CreateICmpNE(l, r);
        return builder.CreateZExt(l, Type::getInt64Ty(getGlobalContext()), "csttmp");
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
    case RETURN: return builder.CreateRet(v);
    case DEREF:
        v = builder.CreateIntToPtr(v, Type::getInt64PtrTy(getGlobalContext()), "ptrtmp");
        return builder.CreateLoad(v, "drftmp");
    case GOTO: return errorV("GOTO not yet implemented"); /* TODO */
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
    insertAll(that->m_vars, Var);
    insertAll(that->m_labels, Label);
}

void Scope::insert(Symbol s) {
    if (contains(s.sym)) {
        fprintf(stderr, "Redefinition of symbol '%s'\n",
                syms.get(s.sym).c_str());
        exit(ERR_SCOPE);
    }
    vector<sym_t> &v = (s.type == Var) ? m_vars : m_labels;
    v.push_back(s.sym);
}

void Scope::insertAll(vector<sym_t> v, enum SymType t) {
    for (unsigned int i = 0; i < v.size(); i++) {
        insert(Symbol(v[i], t));
    }
}

void Scope::insertAll(vector<Symbol> v) {
    for (unsigned int i = 0; i < v.size(); i++) {
        insert(v[i]);
    }
}

int Scope::contains(sym_t s, enum SymType t) const {
    const vector<sym_t> &v = (t == Var) ? m_vars : m_labels;
    for (unsigned int i = 0; i < v.size(); i++) {
        if (s == v[i]) {
            return 1;
        }
    }
    return 0;
}

int Scope::contains(sym_t s) const {
    return (contains(s, Var) || contains(s, Label));
}

const vector<sym_t> &Scope::variables() const {
    return m_vars;
}

const vector<sym_t> &Scope::labels() const {
    return m_labels;
}

string Scope::toString() const {
    stringstream s;
    s << "Current scope: ";
    for (unsigned int i = 0; i < m_vars.size(); i++) {
        s << syms.get(m_vars[i]) << ",";
    }
    s << endl;
    return s.str();
}

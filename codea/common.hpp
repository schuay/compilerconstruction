#include <string>
#include <vector>
#include <llvm/Value.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>

#define ERR_LEX (1)
#define ERR_SYNTAX (2)
#define ERR_SCOPE (3)

using std::string;
using std::vector;
using namespace llvm;

typedef int sym_t;
typedef int op_t;

class SymbolTable;

extern SymbolTable syms;
extern Module *theModule;
extern PassManager *pm;

void printAsm();

enum SymType {
    Var,
    Label
};

/* Only used for passing around symbol lists during symbol
   collection. Necessary because label symbols are treated differently
   than variable symbols. */
class Symbol {
public:
    Symbol(sym_t s, enum SymType t) : sym(s), type(t) {}
    sym_t sym;
    enum SymType type;
};

/* Used only for checking validity of scopes. Code generation
   is simplified by using a global symbol table. */
class Scope {
    vector<sym_t> m_vars;
    vector<sym_t> m_labels;
public:
    void merge(const Scope *that);
    void insert(Symbol s);
    void insertAll(vector<sym_t> v, enum SymType t);
    void insertAll(vector<Symbol> v);
    int contains(sym_t s, enum SymType t) const;
    int contains(sym_t s) const;
    string toString() const;
};

class ExprAST {
public:
    ExprAST() : m_scope(NULL) {}
    virtual ~ExprAST() {}

    /* Prints tree in human readable form. The nest level must
     * be passed to determine indentation. */
    virtual string toString(int level) const = 0;

    /* Sets up scope tables in subtree and returns symbols which
     * are visible in parent. Processes tree bottom-up. */
    virtual vector<Symbol> collectDefinedSymbols() = 0;

    /* Checks for undefined references and merges scopes.
     * Tree is processed top-down. Returns nonzero value
     * if errors occurred. */
    virtual int checkSymbols(Scope *scope) = 0;

    /* Generates LLVM IR code. */
    virtual Value *codegen() = 0;

protected:
    Scope *m_scope;
};

class NumberExprAST : public ExprAST {
    long m_val;
public:
    NumberExprAST(long val) : ExprAST(), m_val(val) {}
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols() { return vector<Symbol>(); }
    virtual int checkSymbols(Scope *) { return 0; }
    virtual Value *codegen();
};

class SymbolExprAST : public ExprAST {
    sym_t m_sym;
    enum SymType m_type;
public:
    SymbolExprAST(sym_t sym, enum SymType type) : ExprAST(), m_sym(sym), m_type(type) {}
    SymbolExprAST(sym_t sym) : ExprAST(), m_sym(sym), m_type(Var) {}
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
    virtual Value *codegen();
};

template <class T>
class UnionList {
public:
    static UnionList<T> *push_back(UnionList<T> *l, T e) {
        if (l == NULL) {
            l = new UnionList<T>;
            l->m_v = vector<T>();
        }
        l->m_v.push_back(e);
        return l;
    }

    const vector<T> &get() const {
        return m_v;
    }
private:
    UnionList() {}
    vector<T> m_v;
};

typedef UnionList<sym_t> SymList;
typedef UnionList<ExprAST *> ExprList;

class FunctionExprAST : public ExprAST {
public:
    FunctionExprAST(sym_t name, SymList *pars, ExprList *stats);
    virtual ~FunctionExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
    virtual Value *codegen();
protected:
    sym_t m_name;
    vector<sym_t> m_pars;
    vector<ExprAST *> m_stats;
};

class StatementExprAST : public ExprAST {
    vector<sym_t> m_labels;
    ExprAST *m_stat;
public:
    StatementExprAST(ExprAST *stat) : ExprAST(),  m_stat(stat) {}
    StatementExprAST(SymList *labels, ExprAST *stat)
        : ExprAST(), m_labels(labels->get()), m_stat(stat) { delete labels; }
    virtual ~StatementExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope) { return m_stat->checkSymbols(scope); }
    virtual Value *codegen();
};

class CallExprAST : public ExprAST {
    sym_t m_callee;
    vector<ExprAST *> m_args;
public:
    CallExprAST(sym_t callee, ExprList *args);
    virtual ~CallExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols() { return vector<Symbol>(); }
    virtual int checkSymbols(Scope *scope);
    virtual Value *codegen();
};

class IfExprAST : public ExprAST {
    ExprAST *m_cond;
    vector<ExprAST *> m_then;
public:
    IfExprAST(ExprAST *cond, ExprList *then);
    virtual ~IfExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
    virtual Value *codegen();
};

class BinaryExprAST : public ExprAST {
    op_t m_op;  /* one of: VAR, '=', '*', '+', AND, OPLESSEQ, '#' */
    ExprAST *m_lhs, *m_rhs;
public:
    BinaryExprAST(op_t op, ExprAST *lhs, ExprAST *rhs)
        : ExprAST(), m_op(op), m_lhs(lhs), m_rhs(rhs) {}
    virtual ~BinaryExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
    virtual Value *codegen();
};

class UnaryExprAST : public ExprAST {
    op_t m_op; /* one of: NOT, UNARYMINUS, DEREF, RETURN, GOTO  */
    ExprAST *m_arg;
public:
    UnaryExprAST(op_t op, ExprAST *arg)
        : ExprAST(), m_op(op), m_arg(arg) {}
    virtual ~UnaryExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols() { return vector<Symbol>(); }
    virtual int checkSymbols(Scope *scope) { return m_arg->checkSymbols(scope); }
    virtual Value *codegen();
};

class SymbolTable {
    vector<string> m_symbols;
public:
    sym_t insert(string s);
    string get(sym_t i) const;
    string toString() const;
};

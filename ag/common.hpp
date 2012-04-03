#include <string>
#include <vector>

#define ERR_LEX (1)
#define ERR_SYNTAX (2)
#define ERR_SCOPE (3)

using std::string;
using std::vector;

typedef int sym_t;
typedef int op_t;

class SymbolTable;

extern SymbolTable syms;

enum SymType {
    Var,
    Label
};

class Symbol {
public:
    Symbol(sym_t s, enum SymType t) : sym(s), type(t) {}
    sym_t sym;
    enum SymType type;
};

class Scope {
    vector<sym_t> m_symbols;
public:
    void merge(const Scope *that);
    void insert(sym_t s);
    void insertAll(vector<sym_t> v);
    void insertAll(vector<Symbol> v);
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
};

class SymbolExprAST : public ExprAST {
    sym_t m_sym;
public:
    SymbolExprAST(sym_t sym) : ExprAST(), m_sym(sym) {}
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
};

class ListExprAST : public ExprAST {
    vector<ExprAST *> m_exprs;
    ListExprAST() {}
public:
    ListExprAST(vector<ExprAST *> exprs) : ExprAST(), m_exprs(exprs) {}
    virtual ~ListExprAST();
    static ListExprAST *push_back(ListExprAST *l, ExprAST *e);
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
};

class SymListExprAST : public ExprAST {
    vector<sym_t> m_syms;
    SymListExprAST() {}
public:
    SymListExprAST(vector<sym_t> syms) : ExprAST(), m_syms(syms) {}
    static SymListExprAST *push_back(SymListExprAST *l, sym_t e);
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *) { return 0; }
};

class FunctionExprAST : public ExprAST {
public:
    FunctionExprAST(sym_t name, SymListExprAST *pars, ListExprAST *stats)
        : ExprAST(), m_name(name), m_pars(pars), m_stats(stats) {}
    virtual ~FunctionExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
protected:
    sym_t m_name;
    SymListExprAST *m_pars;
    ListExprAST *m_stats;
};

class StatementExprAST : public ExprAST {
    SymListExprAST *m_labels;
    ExprAST *m_stat;
public:
    StatementExprAST(ExprAST *stat) : ExprAST(), m_labels(NULL), m_stat(stat) {}
    StatementExprAST(SymListExprAST *labels, ExprAST *stat)
        : ExprAST(), m_labels(labels), m_stat(stat) {}
    virtual ~StatementExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope) { return m_stat->checkSymbols(scope); }
};

class CallExprAST : public ExprAST {
    sym_t m_callee;
    ListExprAST *m_args;
public:
    CallExprAST(sym_t callee, ListExprAST *args)
        : ExprAST(), m_callee(callee), m_args(args) {}
    virtual ~CallExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols() { return vector<Symbol>(); }
    virtual int checkSymbols(Scope *scope) { return m_args->checkSymbols(scope); }
};

class BinaryExprAST : public ExprAST {
    op_t m_op;  /* one of: IF, VAR, '=', '*', '+', AND, OPLESSEQ, '#' */ 
    ExprAST *m_lhs, *m_rhs;
public:
    BinaryExprAST(op_t op, ExprAST *lhs, ExprAST *rhs)
        : ExprAST(), m_op(op), m_lhs(lhs), m_rhs(rhs) {}
    virtual ~BinaryExprAST();
    virtual string toString(int level) const;
    virtual vector<Symbol> collectDefinedSymbols();
    virtual int checkSymbols(Scope *scope);
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
};

class SymbolTable {
    vector<string> m_symbols;
public:
    sym_t insert(string s);
    string get(sym_t i) const;
    string toString() const;
};
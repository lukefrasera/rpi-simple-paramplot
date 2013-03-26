#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include <map>
#include <string>
#include <vector>

struct Token
{
    enum type_t
    {
        INVALID, END,
        NUM, IDENT,
        EQUAL, NOT_EQUAL, LESS_THAN, LESS_EQUAL, GREATER_EQUAL, GREATER_THAN,
        PLUS, MINUS, ASTERISK, SLASH, CARET,
        PAREN_OPEN, PAREN_CLOSE, COMMA,
        QUESTION, COLON
    } type;

    double num;
    std::string ident;

    Token() : type(INVALID) {}
    Token(type_t type) : type(type) {}
    Token(double num) : type(NUM), num(num) {}
    Token(const std::string& ident) : type(IDENT), ident(ident) {}
};

class Tokenizer
{
public:
    Tokenizer(const std::string& input) : input(input), pos(0), pos_tokstart(0) {}

    Token read_token();
    size_t get_pos_tokstart() { return pos_tokstart; }

private:
    int peek() { return pos < input.length() ? input[pos] : -1; }

    double parse_number();
    std::string parse_ident();

    std::string input;
    size_t pos;
    size_t pos_tokstart;
};


struct Operation
{
    enum op_t
    {
        PUSH_NUM, PUSH_VAR,
        EQ, NEQ, LT, LE, GE, GT,
        IFELSE,
        ADD, SUB, MUL, DIV,
        POW, NEG, ABS,
        SIN, COS, TAN,
        EXP
    } op;

    union
    {
        double num;
        int var_idx;
    };

    Operation(op_t op) : op(op) {}
    Operation(double num) : op(PUSH_NUM), num(num) {}
    Operation(int var_idx) : op(PUSH_VAR), var_idx(var_idx) {}
};


class Evaluator
{
public:
    typedef std::vector<std::string> varlist_t;
    typedef std::map<std::string, double> constmap_t;

    Evaluator(const std::string& formula, const varlist_t& varlist, const constmap_t& constmap = constmap_t());

    double evaluate(const std::vector<double>& vars);

private:
    const Token& next_token() { return cur_token = tokenizer.read_token(); }
    void parse_expr();  // highest level
    void parse_ifelse();  // ternary (a ? b : c) operator
    void parse_comparison();  // equal, less, greater ...
    void parse_sum();  // plus, minus
    void parse_product();  // times, divide
    void parse_power();  // for powers
    void parse_factor();  // for numbers, identifiers and parenthesized expressions

    Tokenizer tokenizer;
    Token cur_token;
    varlist_t varlist;
    constmap_t constmap;
    std::vector<Operation> op_list;
};

#endif  // EVALUATOR_HPP

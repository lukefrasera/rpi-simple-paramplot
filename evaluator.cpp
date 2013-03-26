#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stack>
#include "evaluator.hpp"
using namespace std;

Token Tokenizer::read_token()
{
    int c;

    while(isblank(c = peek())) ++ pos;

    pos_tokstart = pos;

    if(isdigit(c) || c == '.') return Token(parse_number());
    if(isalpha(c)) return Token(parse_ident());

    // Comparison operators:
    if(c == '=')
    {
        ++ pos;
        if(peek() == '=')
        {
            ++ pos;
            return Token(Token::EQUAL);
        }
        else
        {
            return Token(Token::INVALID);
        }
    }
    if(c == '!')
    {
        ++ pos;
        if(peek() == '=')
        {
            ++ pos;
            return Token(Token::NOT_EQUAL);
        }
        else
        {
            return Token(Token::INVALID);
        }
    }
    if(c == '<')
    {
        ++ pos;
        if(peek() == '=')
        {
            ++ pos;
            return Token(Token::LESS_EQUAL);
        }
        else
        {
            return Token(Token::LESS_THAN);
        }
    }
    if(c == '>')
    {
        ++ pos;
        if(peek() == '=')
        {
            ++ pos;
            return Token(Token::GREATER_EQUAL);
        }
        else
        {
            return Token(Token::GREATER_THAN);
        }
    }

    // Arithmetic operators:
    if(c == '+') { ++ pos; return Token(Token::PLUS); }
    if(c == '-') { ++ pos; return Token(Token::MINUS); }
    if(c == '*') { ++ pos; return Token(Token::ASTERISK); }
    if(c == '/') { ++ pos; return Token(Token::SLASH); }
    if(c == '^') { ++ pos; return Token(Token::CARET); }

    // Grouping symbols:
    if(c == '(') { ++ pos; return Token(Token::PAREN_OPEN); }
    if(c == ')') { ++ pos; return Token(Token::PAREN_CLOSE); }
    if(c == ',') { ++ pos; return Token(Token::COMMA); }

    if(c == '?') { ++ pos; return Token(Token::QUESTION); }
    if(c == ':') { ++ pos; return Token(Token::COLON); }

    // Special:
    if(c == -1) { return Token(Token::END); }
    return Token(Token::INVALID);
}

double Tokenizer::parse_number()
{
    int pre_val = 0;
    int post_val = 0;
    double post_factor = 1.0;

    char c;

    // Get pre-dot number:
    while(isdigit(c = peek()))
    {
        ++ pos;

        pre_val *= 10;
        pre_val += c - '0';
    }

    if(peek() == '.')
    {
        ++ pos;

        // Get post-dot number:
        while(isdigit(c = peek()))
        {
            ++ pos;

            post_val *= 10;
            post_val += c - '0';
            post_factor /= 10.0;
        }
    }

    return pre_val + post_factor * post_val;
}

string Tokenizer::parse_ident()
{
    string result;

    char c;

    // Get name:
    while(isalnum(c = peek()) || c == '_' || c == '\'')
    {
        ++ pos;
        result += c;
    }

    return result;
}


Evaluator::Evaluator(const std::string& formula, const varlist_t& varlist, const constmap_t& constmap)
 : tokenizer(formula), varlist(varlist), constmap(constmap)
{
    try
    {
        next_token();
        parse_expr();
        if(cur_token.type != Token::END) throw string("unexpected extra token");
    }
    catch(const string& e)
    {
        stringstream msg;
        msg << "at position " << tokenizer.get_pos_tokstart() << ": " << e;
        throw msg.str();
    }
}

double Evaluator::evaluate(const std::vector<double>& vars)
{
    stack<double, vector<double> > value_stack;

    typedef vector<Operation>::const_iterator IT;

    for(IT it = op_list.begin(); it != op_list.end(); ++it)
    {
        double a, b, c;

        switch(it->op)
        {
        case Operation::PUSH_NUM:
            value_stack.push(it->num);
            break;
                
        case Operation::PUSH_VAR:
            value_stack.push(vars[it->var_idx]);
            break;
            

        case Operation::EQ:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a == b);
            break;

        case Operation::NEQ:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a != b);
            break;

        case Operation::LT:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a < b);
            break;

        case Operation::LE:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a <= b);
            break;

        case Operation::GE:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a >= b);
            break;

        case Operation::GT:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a > b);
            break;
            

        case Operation::IFELSE:
            c = value_stack.top();  value_stack.pop();
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a ? b : c);
            break;


        case Operation::ADD:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a + b);
            break;

        case Operation::SUB:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a - b);
            break;
            
        case Operation::MUL:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a * b);
            break;
            
        case Operation::DIV:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(a / b);
            break;
            
        case Operation::POW:
            b = value_stack.top();  value_stack.pop();
            a = value_stack.top();  value_stack.pop();
            value_stack.push(pow(a, b));
            break;
            
        case Operation::NEG:
            a = value_stack.top();  value_stack.pop();
            value_stack.push(-a);
            break;
            
        case Operation::ABS:
            a = value_stack.top();  value_stack.pop();
            value_stack.push(abs(a));
            break;
            
        case Operation::SIN:
            a = value_stack.top();  value_stack.pop();
            value_stack.push(sin(a));
            break;
            
        case Operation::COS:
            a = value_stack.top();  value_stack.pop();
            value_stack.push(cos(a));
            break;
            
        case Operation::TAN:
            a = value_stack.top();  value_stack.pop();
            value_stack.push(tan(a));
            break;
            
        case Operation::EXP:
            a = value_stack.top();  value_stack.pop();
            value_stack.push(exp(a));
            break;
        }
    }

    return value_stack.top();
}

void Evaluator::parse_expr()
{
    parse_ifelse();
}

void Evaluator::parse_ifelse()
{
    parse_comparison();

    if(cur_token.type == Token::QUESTION)
    {
        next_token();
        parse_ifelse();
        if(cur_token.type != Token::COLON)
        {
            throw string("colon expected");
        }
        next_token();
        parse_ifelse();
        op_list.push_back(Operation(Operation::IFELSE));
    }
}

void Evaluator::parse_comparison()
{
    parse_sum();

    if(cur_token.type == Token::EQUAL)
    {
        next_token();
        parse_sum();
        op_list.push_back(Operation(Operation::EQ));
    }
    else if(cur_token.type == Token::NOT_EQUAL)
    {
        next_token();
        parse_sum();
        op_list.push_back(Operation(Operation::NEQ));
    }
    else if(cur_token.type == Token::LESS_THAN)
    {
        next_token();
        parse_sum();
        op_list.push_back(Operation(Operation::LT));
    }
    else if(cur_token.type == Token::LESS_EQUAL)
    {
        next_token();
        parse_sum();
        op_list.push_back(Operation(Operation::LE));
    }
    else if(cur_token.type == Token::GREATER_EQUAL)
    {
        next_token();
        parse_sum();
        op_list.push_back(Operation(Operation::GE));
    }
    else if(cur_token.type == Token::GREATER_THAN)
    {
        next_token();
        parse_sum();
        op_list.push_back(Operation(Operation::GT));
    }
}

void Evaluator::parse_sum()
{
    parse_product();

    while(cur_token.type == Token::PLUS || cur_token.type == Token::MINUS)
    {
        if(cur_token.type == Token::PLUS)
        {
            // Addition:
            next_token();
            parse_product();
            op_list.push_back(Operation(Operation::ADD));
        }
        else
        {
            // Subtraction:
            next_token();
            parse_product();
            op_list.push_back(Operation(Operation::SUB));
        }
    }
}

void Evaluator::parse_product()
{
    parse_power();

    while(cur_token.type == Token::ASTERISK || cur_token.type == Token::SLASH)
    {
        if(cur_token.type == Token::ASTERISK)
        {
            // Multiplication:
            next_token();
            parse_power();
            op_list.push_back(Operation(Operation::MUL));
        }
        else
        {
            // Division:
            next_token();
            parse_power();
            op_list.push_back(Operation(Operation::DIV));
        }
    }
}

void Evaluator::parse_power()
{
    // Get sign:
    bool negative = false;
    while(cur_token.type == Token::PLUS || cur_token.type == Token::MINUS)
    {
        if(cur_token.type == Token::MINUS) negative = !negative;
        next_token();
    }

    parse_factor();

    while(cur_token.type == Token::CARET)
    {
        // Exponentiation:
        next_token();
        parse_factor();
        op_list.push_back(Operation(Operation::POW));
    }

    if(negative) op_list.push_back(Operation(Operation::NEG));
}

void Evaluator::parse_factor()
{
    if(cur_token.type == Token::IDENT)
    {
        const string& id = cur_token.ident;
        varlist_t::const_iterator varlist_it;
        constmap_t::const_iterator constmap_it;

        if(id == "abs")
        {
            next_token();
            parse_factor();
            op_list.push_back(Operation(Operation::ABS));
        }
        else if(id == "sin")
        {
            next_token();
            parse_factor();
            op_list.push_back(Operation(Operation::SIN));
        }
        else if(id == "cos")
        {
            next_token();
            parse_factor();
            op_list.push_back(Operation(Operation::COS));
        }
        else if(id == "tan")
        {
            next_token();
            parse_factor();
            op_list.push_back(Operation(Operation::TAN));
        }
        else if(id == "exp")
        {
            next_token();
            parse_factor();
            op_list.push_back(Operation(Operation::EXP));
        }
        else if((varlist_it = find(varlist.begin(), varlist.end(), id)) != varlist.end())
        {
            op_list.push_back(Operation(static_cast<int>(varlist_it - varlist.begin())));
            next_token();
        }
        else if((constmap_it = constmap.find(id)) != constmap.end())
        {
            op_list.push_back(Operation(constmap_it->second));
            next_token();
        }
        else
            throw string("unknown identifier \"") + cur_token.ident + '"';
    }
    else if(cur_token.type == Token::NUM)
    {
        op_list.push_back(Operation(cur_token.num));
        next_token();
    }
    else if(cur_token.type == Token::PAREN_OPEN)
    {
        next_token();
        parse_expr();

        // Get closing paren:
        if(cur_token.type != Token::PAREN_CLOSE)
        {
            throw string("closing parenthesis expected");
        }
        next_token();
    }
    else
    {
        throw string("number or parenthesis expected");
    }
}

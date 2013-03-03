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

    if(c == '+') { ++ pos; return Token(Token::PLUS); }
    if(c == '-') { ++ pos; return Token(Token::MINUS); }
    if(c == '*') { ++ pos; return Token(Token::ASTERISK); }
    if(c == '/') { ++ pos; return Token(Token::SLASH); }
    if(c == '^') { ++ pos; return Token(Token::CARET); }

    if(c == '(') { ++ pos; return Token(Token::PAREN_OPEN); }
    if(c == ')') { ++ pos; return Token(Token::PAREN_CLOSE); }
    if(c == ',') { ++ pos; return Token(Token::COMMA); }

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
        double a, b;

        switch(it->op)
        {
        case Operation::PUSH_NUM:
            value_stack.push(it->num);
            break;
                
        case Operation::PUSH_VAR:
            value_stack.push(vars[it->var_idx]);
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
        }
    }

    return value_stack.top();
}

void Evaluator::parse_expr()
{
    parse_term();

    while(cur_token.type == Token::PLUS || cur_token.type == Token::MINUS)
    {
        if(cur_token.type == Token::PLUS)
        {
            // Addition:
            next_token();
            parse_term();
            op_list.push_back(Operation(Operation::ADD));
        }
        else
        {
            // Subtraction:
            next_token();
            parse_term();
            op_list.push_back(Operation(Operation::SUB));
        }
    }
}

void Evaluator::parse_term()
{
    parse_coefficient();

    while(cur_token.type == Token::ASTERISK || cur_token.type == Token::SLASH)
    {
        if(cur_token.type == Token::ASTERISK)
        {
            // Multiplication:
            next_token();
            parse_coefficient();
            op_list.push_back(Operation(Operation::MUL));
        }
        else
        {
            // Division:
            next_token();
            parse_coefficient();
            op_list.push_back(Operation(Operation::DIV));
        }
    }
}

void Evaluator::parse_coefficient()
{
    parse_factor();

    while(cur_token.type == Token::CARET)
    {
        // Exponentiation:
        next_token();
        parse_factor();
        op_list.push_back(Operation(Operation::POW));
    }
}

void Evaluator::parse_factor()
{
    // Get sign:
    bool negative = false;
    while(cur_token.type == Token::PLUS || cur_token.type == Token::MINUS)
    {
        if(cur_token.type == Token::MINUS) negative = !negative;
        next_token();
    }

    if(cur_token.type == Token::IDENT)
    {
        const string& id = cur_token.ident;
        varlist_t::const_iterator varlist_it;
        constmap_t::const_iterator constmap_it;

        if(id == "sin")
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

    if(negative) op_list.push_back(Operation(Operation::NEG));
}

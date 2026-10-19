#include "sexpression.h"
#include "token.h"

#include <stdexcept>
#include <iostream>
#include <string>
#include <cctype>
#include <fstream>
#include <sstream>

// global variable holder rho
SExpression* rho = new SExpression();

// local variable stack
SExpression* stack = new SExpression();

SExpression* makeTrue()
{
    SExpression* result = new SExpression();

    result->type = ExpressionType::Atom;
    result->atom = "T";

    return result;
}

SExpression* makeFalse()
{
    return new SExpression();
}

void SExpression::parse(const std::vector<Token>& tokens, size_t& position)
{

    if (position >= tokens.size())
    {
        throw std::runtime_error("Unexpected end of input");
    }

    // <EXPR> => <ATOM>
    if (tokens[position].type == TokenType::Atom)
    {
        type = ExpressionType::Atom;
        atom = tokens[position].value;

        position++;
        return;
    }

    // <EXPR> => ( <LIST>
    if (tokens[position].type == TokenType::LeftParen)
    {
        position++; // consume '('

        parseList(tokens, position);
        return;
    }

    // expr -> '(...)
    if (tokens[position].type == TokenType::Quote)
    {
        position++; // consume

        type = ExpressionType::Pair;

        car = new SExpression();
        car->type = ExpressionType::Atom;
        car->atom = "quote";

        cdr = new SExpression();
        cdr->type = ExpressionType::Pair;

        cdr->car = new SExpression();
        cdr->car->parse(tokens, position);

        cdr->cdr = new SExpression();
        cdr->cdr->type = ExpressionType::Nil;

        return;
    }

    throw std::runtime_error("Unexpected ')'");
}

void SExpression::parseList(const std::vector<Token>& tokens, size_t& position)
{
    if (position >= tokens.size())
    {
        throw std::runtime_error("Missing ')'");
    }

    // <LIST> => )
    if (tokens[position].type == TokenType::RightParen)
    {
        type = ExpressionType::Nil;
        position++; // consume ')'
        return;
    }

    // <LIST> => <EXPR> <LIST>

    type = ExpressionType::Pair;

    car = new SExpression();
    car->parse(tokens, position);

    cdr = new SExpression();
    cdr->parseList(tokens, position);
}

bool SExpression::printable()
{
    if (car->atom == "set" || car->atom == "def" || car->atom == "function" || car->atom == "use")
    {
        return false;
    }

    return true;
}

void SExpression::print(std::ostream &output)
{
    bool shouldPrint = true;
    if (isPair() && car->isAtom())
    {
        shouldPrint = printable();
    }

    if (shouldPrint)
    {
        if (isAtom())
        {
            output << atom;
            return;
        }

        if (isNil())
        {
            output << "()";
            return;
        }

        if (isPair())
        {
            output << "(";
            printList(output);
            output << ")";
        }
    }
}

void SExpression::printList(std::ostream &output)
{
    car->print(output);

    if (cdr->isNil())
    {
        return;
    }

    if (cdr->isPair())
    {
        output << " ";
        cdr->printList(output);
        return;
    }

    output << " . ";

    cdr->print(output);
}

SExpression::~SExpression()
{
    delete car;
    delete cdr;
}

SExpression* SExpression::clone() const
{
    SExpression* copy = new SExpression();

    copy->type = type;
    copy->atom = atom;

    if (car != nullptr)
    {
        copy->car = car->clone();
    }

    if (cdr != nullptr)
    {
        copy->cdr = cdr->clone();
    }

    return copy;
}

bool SExpression::isAtom() const
{
    return type == ExpressionType::Atom;
}

bool SExpression::isNil() const
{
    return type == ExpressionType::Nil;
}

bool SExpression::isPair() const
{
    return type == ExpressionType::Pair;
}

SExpression* lookup(const SExpression& symbol)
{
    SExpression* current = stack;

    while (current->isPair())
    {
        SExpression* frame = current->car;

        SExpression* names = frame->car;
        SExpression* values = frame->cdr->car;

        while (names->isPair() && values->isPair())
        {
            SExpression* name = names->car;
            SExpression* val = values->car;

            if (name->atom == symbol.atom)
            {
                return val->clone();
            }

            names = names->cdr;
            values = values->cdr;
        }

        current = current->cdr;
    }

    current = rho;

    while (current->isPair())
    {
        SExpression* bind = current->car;

        SExpression* name = bind->car;
        SExpression* value = bind->cdr->car;

        if (name->atom == symbol.atom)
        {
            return value->clone();
        }

        current = current->cdr;
    }

    return symbol.clone();
}

SExpression* car(const SExpression& expr)
{
    if (expr.type != ExpressionType::Pair)
    {
        throw std::runtime_error("car expects a pair");
    }

    return expr.car->clone();
}

SExpression* cdr(const SExpression& expr)
{
    if (expr.type != ExpressionType::Pair)
    {
        throw std::runtime_error("cdr expects a pair");
    }

    return expr.cdr->clone();
}

SExpression* cons(const SExpression& first, const SExpression& second)
{
    SExpression* result = new SExpression();

    result->type = ExpressionType::Pair;

    result->car = first.clone();
    result->cdr = second.clone();

    return result;
}

SExpression* quote(const SExpression& expr)
{
    return expr.clone();
}

SExpression* set(const SExpression& name, const SExpression& value)
{
    SExpression nil;

    SExpression* val = cons(value, nil);
    SExpression* assignment = cons(name, *val);

    delete val;

    return assignment;
}

SExpression* nilPredicate(const SExpression& expr)
{
    if (expr.isNil())
    {
        return makeTrue();
    }

    return makeFalse();
}

SExpression* atomPredicate(const SExpression& expr)
{
    if (expr.isAtom())
    {
        return makeTrue();
    }

    return makeFalse();
}

SExpression* numberPredicate(const SExpression& expr)
{
    if (!expr.isAtom())
    {
        return makeFalse();
    }

    const std::string& value = expr.atom;

    if (value.empty())
    {
        return makeFalse();
    }

    size_t start = 0;

    if (value[0] == '+' || value[0] == '-')
    {
        if (value.size() == 1)
        {
            return makeFalse();
        }

        start = 1;
    }

    for (size_t i = start; i < value.size(); i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(value[i])))
        {
            return makeFalse();
        }
    }

    return makeTrue();
}

SExpression* andPredicate(const SExpression& first, const SExpression& second)
{
    SExpression* arg1 = eval(first);

    if (arg1->isNil())
    {
        delete arg1;
        return makeFalse();
    }

    SExpression* arg2 = eval(second);

    if (arg2->isNil())
    {
        delete arg1;
        delete arg2;
        return makeFalse();
    }

    delete arg1;
    delete arg2;

    return makeTrue();
}

SExpression* orPredicate(const SExpression& first, const SExpression& second)
{
    SExpression* arg1 = eval(first);

    if (!arg1->isNil())
    {
        delete arg1;
        return makeTrue();
    }

    SExpression* arg2 = eval(second);

    if (!arg2->isNil())
    {
        delete arg1;
        delete arg2;
        return makeTrue();
    }

    delete arg1;
    delete arg2;

    return makeFalse();
}

SExpression* listPredicate(const SExpression& expr)
{
    if (expr.isPair())
    {
        return makeTrue();
    }

    return makeFalse();
}

SExpression* eqPredicate(const SExpression& first, const SExpression& second)
{
    if (!first.isAtom() || !second.isAtom())
    {
        return makeFalse();
    }

    if (first.atom != second.atom)
    {
        return makeFalse();
    }


    return makeTrue();
}

SExpression* ifPredicate(const SExpression& a0, const SExpression& a1, const SExpression& a2)
{
    SExpression* condition = eval(a0);

    if (condition->isNil())
    {
        delete condition;
        return eval(a2);
    } 
    else
    {
        delete condition;
        return eval(a1);
    }
}

SExpression* condPredicate(const SExpression& list)
{
    const SExpression* current = &list;

    while (!current->isNil())
    {
        SExpression* condition = eval(*current->car);

        if (!condition->isNil())
        {
            delete condition;

            return eval(*current->cdr->car);
        }

        delete condition;

        current = current->cdr->cdr;
    }

    throw std::runtime_error("last expression must be 'T");
}

int toNumber(SExpression* expr)
{
    return std::stoi(expr->atom);
}

SExpression* makeNumber(int num)
{
    SExpression* result = new SExpression();

    result->atom = std::to_string(num);
    result->type = ExpressionType::Atom;

    return result;
}

bool isNumber(const SExpression& expr)
{
    SExpression* condition = numberPredicate(expr);

    if (condition->isNil())
    {
        delete condition;
        return false;
    }

    delete condition;
    return true;
}

SExpression* lessThan(const SExpression& arg1, const SExpression& arg2)
{
    SExpression* first = eval(arg1);

    if (!isNumber(*first))
    {
        delete first;
        throw std::runtime_error("first operand is not a number");
    }

    int left = toNumber(first);

    SExpression* second = eval(arg2);

    if (!isNumber(*second))
    {
        delete first;
        delete second;

        throw std::runtime_error("second operand is not a number");
    }

    int right = toNumber(second);

    delete first;
    delete second;

    bool lessThan = left < right;

    if (lessThan)
    {
        return makeTrue();
    }

    return makeFalse();
}

SExpression* rem(const SExpression& arg1, const SExpression& arg2)
{
    SExpression* first = eval(arg1);

    if (!isNumber(*first))
    {
        delete first;
        throw std::runtime_error("first operand is not a number");
    }

    int left = toNumber(first);

    SExpression* second = eval(arg2);

    if (!isNumber(*second))
    {
        delete first;
        delete second;

        throw std::runtime_error("second operand is not a number");
    }

    int right = toNumber(second);

    delete first;
    delete second;

    SExpression* result = makeNumber(left % right);

    return result;
}

SExpression* div(const SExpression& arg1, const SExpression& arg2)
{
    SExpression* first = eval(arg1);

    if (!isNumber(*first))
    {
        delete first;
        throw std::runtime_error("first operand is not a number");
    }

    int left = toNumber(first);

    SExpression* second = eval(arg2);

    if (!isNumber(*second))
    {
        delete first;
        delete second;

        throw std::runtime_error("second operand is not a number");
    }

    int right = toNumber(second);

    delete first;
    delete second;

    SExpression* result = makeNumber(left / right);

    return result;
}

SExpression* mul(const SExpression& arg1, const SExpression& arg2)
{
    SExpression* first = eval(arg1);

    if (!isNumber(*first))
    {
        delete first;
        throw std::runtime_error("first operand is not a number");
    }

    int left = toNumber(first);

    SExpression* second = eval(arg2);

    if (!isNumber(*second))
    {
        delete first;
        delete second;

        throw std::runtime_error("second operand is not a number");
    }

    int right = toNumber(second);

    delete first;
    delete second;

    SExpression* result = makeNumber(left * right);

    return result;
}

SExpression* sub(const SExpression& arg1, const SExpression& arg2)
{
    SExpression* first = eval(arg1);

    if (!isNumber(*first))
    {
        delete first;
        throw std::runtime_error("first operand is not a number");
    }

    int left = toNumber(first);

    SExpression* second = eval(arg2);

    if (!isNumber(*second))
    {
        delete first;
        delete second;

        throw std::runtime_error("second operand is not a number");
    }

    int right = toNumber(second);

    delete first;
    delete second;

    SExpression* result = makeNumber(left - right);

    return result;
}

SExpression* add(const SExpression& arg1, const SExpression& arg2)
{
    SExpression* first = eval(arg1);

    if (!isNumber(*first))
    {
        delete first;
        throw std::runtime_error("first operand is not a number");
    }

    int left = toNumber(first);

    SExpression* second = eval(arg2);

    if (!isNumber(*second))
    {
        delete first;
        delete second;

        throw std::runtime_error("second operand is not a number");
    }

    int right = toNumber(second);

    delete first;
    delete second;

    SExpression* result = makeNumber(left + right);

    return result;
}

SExpression* evalArgs(const SExpression& arguments)
{
    if (arguments.isNil())
    {
        return arguments.clone();
    }

    SExpression* first = eval(*arguments.car);
    SExpression* rest = evalArgs(*arguments.cdr);

    SExpression* result = cons(*first, *rest);

    delete first;
    delete rest;

    return result;
}

void pushStack(const SExpression& names, const SExpression& values)
{
    SExpression nil;

    SExpression* valueList = cons(values, nil);
    SExpression* frame = cons(names, *valueList);

    SExpression* newStack = cons(*frame, *stack);
    delete stack;
    stack = newStack;

    delete frame;
    delete valueList;
}

void popStack()
{
    if (stack->isNil())
    {
        return;
    }

    SExpression* newStack = stack->cdr->clone();

    delete stack;
    stack = newStack;
}

SExpression* eval(const SExpression& expr)
{
    if (expr.isNil())
    {
        return expr.clone();
    }

    if (expr.isAtom())
    {
        return lookup(expr);
    }

    if (!expr.isPair())
    {
        throw std::runtime_error("Invalid expression");
    }

    if (!expr.car->isAtom())
    {
        throw std::runtime_error("Expected function name");
    }

    std::string function = expr.car->atom;

    if (function == "use")
    {
        std::string libName = expr.cdr->car->atom;

        std::ifstream library(libName); 

        std::stringstream buffer;
        buffer << library.rdbuf();

        std::string input = buffer.str();

        std::vector<Token> tokens = tokenize(input);

        library.close();

        size_t position = 0;

        while (position < tokens.size())
        {
            SExpression libExpr;
            libExpr.parse(tokens, position);

            eval(libExpr);
        }
    }

    if (function == "quote")
    {
        return quote(*expr.cdr->car);
    }

    if (function == "car")
    {
        SExpression* argument = eval(*expr.cdr->car);

        SExpression* result = car(*argument);

        delete argument;

        return result;
    }

    if (function == "cdr")
    {
        SExpression* argument = eval(*expr.cdr->car);

        SExpression* result = cdr(*argument);

        delete argument;

        return result;
    }

    if (function == "cons")
    {
        SExpression* first = eval(*expr.cdr->car);

        SExpression* second = eval(*expr.cdr->cdr->car);

        SExpression* result = cons(*first, *second);

        delete first;
        delete second;

        return result;
    }

    if (function == "eval")
    {
        SExpression* argument = eval(*expr.cdr->car);

        SExpression* result = eval(*argument);

        delete argument;

        return result;
    }

    if (function == "set")
    {
        const SExpression* name = expr.cdr->car;

        SExpression* value = eval(*expr.cdr->cdr->car);

        SExpression* bind = set(*name, *value);
        SExpression* newRho = cons(*bind, *rho);

        delete rho;
        rho = newRho;

        delete bind;

        return value;
    }

    if (function == "nil?")
    {
        SExpression* argument = eval(*expr.cdr->car);
        SExpression* result = nilPredicate(*argument);

        delete argument;

        return result;
    }

    if (function == "atom?")
    {
        SExpression* argument = eval(*expr.cdr->car);
        SExpression* result = atomPredicate(*argument);

        delete argument;

        return result;
    }

    if (function == "list?")
    {
        SExpression* argument = eval(*expr.cdr->car);
        SExpression* result = listPredicate(*argument);

        delete argument;

        return result;
    }

    if (function == "not?")
    {
        SExpression* argument = eval(*expr.cdr->car);
        SExpression* result = nilPredicate(*argument);

        delete argument;

        return result;
    }

    if (function == "number?")
    {
        SExpression* argument = eval(*expr.cdr->car);
        SExpression* result = numberPredicate(*argument);

        delete argument;

        return result;
    }

    if (function == "and?")
    {
        SExpression& first = *expr.cdr->car;
        SExpression& second = *expr.cdr->cdr->car;
        SExpression* result = andPredicate(first, second);

        return result;
    }

    if (function == "or?")
    {
        SExpression& first = *expr.cdr->car;
        SExpression& second = *expr.cdr->cdr->car;
        SExpression* result = orPredicate(first, second);

        return result;
    }

    if (function == "eq?")
    {
        SExpression* first = eval(*expr.cdr->car);
        SExpression* second = eval(*expr.cdr->cdr->car);
        SExpression* result = eqPredicate(*first, *second);

        delete first;
        delete second;

        return result;
    }

    if (function == "if")
    {
        const SExpression& a0 = *expr.cdr->car;
        const SExpression& a1 = *expr.cdr->cdr->car;
        const SExpression& a2 = *expr.cdr->cdr->cdr->car;

       return ifPredicate(a0, a1, a2);
    }

    if (function == "cond")
    {
        const SExpression* list = expr.cdr->car;

        return condPredicate(*list);
    }

    if (function == "add")
    {
        const SExpression& arg1 = *expr.cdr->car;
        const SExpression& arg2 = *expr.cdr->cdr->car;

        SExpression* result = add(arg1, arg2);

        return result;
    }

    if (function == "sub")
    {
        const SExpression& arg1 = *expr.cdr->car;
        const SExpression& arg2 = *expr.cdr->cdr->car;

        SExpression* result = sub(arg1, arg2);

        return result;
    }

    if (function == "mul")
    {
        const SExpression& arg1 = *expr.cdr->car;
        const SExpression& arg2 = *expr.cdr->cdr->car;

        SExpression* result = mul(arg1, arg2);

        return result;
    }

    if (function == "div")
    {
        const SExpression& arg1 = *expr.cdr->car;
        const SExpression& arg2 = *expr.cdr->cdr->car;

        SExpression* result = div(arg1, arg2);

        return result;
    }

    if (function == "rem")
    {
        const SExpression& arg1 = *expr.cdr->car;
        const SExpression& arg2 = *expr.cdr->cdr->car;

        SExpression* result = rem(arg1, arg2);

        return result;
    }

    if (function == "lt")
    {
        const SExpression& arg1 = *expr.cdr->car;
        const SExpression& arg2 = *expr.cdr->cdr->car;

        SExpression* result = lessThan(arg1, arg2);

        return result;
    }

    if (function == "def")
    {
        SExpression* funcName = expr.cdr->car;

        SExpression nil;

        SExpression* args = expr.cdr->cdr->car;
        SExpression* body = expr.cdr->cdr->cdr->car;

        SExpression* temp = cons(*body, nil);
        SExpression* argsAndBody = cons(*args, *temp);

        SExpression functionAtom;
        functionAtom.atom = "function";
        functionAtom.type = ExpressionType::Atom;

        SExpression* def = cons(functionAtom, *argsAndBody);

        delete temp;
        delete argsAndBody;

        SExpression* bind = set(*funcName, *def);
        SExpression* newRho = cons(*bind, *rho);

        delete rho;
        rho = newRho;

        delete bind;

        return def;
    }

    if (function == "function")
    {
        return expr.clone();
    }

    SExpression* funcDef = lookup(*expr.car);

    if (funcDef->isPair() && funcDef->car->isAtom() && funcDef->car->atom == "function")
    {
        SExpression* arguments = evalArgs(*expr.cdr);

        pushStack(*funcDef->cdr->car, *arguments);

        SExpression* body = funcDef->cdr->cdr->car;

        SExpression* result = eval(*body);

        popStack();

        delete arguments;
        delete funcDef;

        return result;
    }

    delete funcDef;

    return expr.clone();
}

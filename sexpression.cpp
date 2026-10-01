#include "sexpression.h"

#include <stdexcept>
#include <iostream>
#include <string>

// global variable holder rho
SExpression* rho = new SExpression();

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

void SExpression::print(std::ostream &output)
{
    if (type == ExpressionType::Atom)
    {
        output << atom;
        return;
    }

    if (type == ExpressionType::Nil)
    {
        output << "()";
        return;
    }

    if (type == ExpressionType::Pair)
    {
        output << "(";
        printList(output);
        output << ")";
    }
}

void SExpression::printList(std::ostream &output)
{
    car->print(output);

    if (cdr->type == ExpressionType::Nil)
    {
        return;
    }

    if (cdr->type == ExpressionType::Pair)
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
    SExpression* current = rho;

    while (current->type == ExpressionType::Pair)
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

SExpression* listPredicate(const SExpression& expr)
{
    if (expr.isPair())
    {
        return makeTrue();
    }

    return makeFalse();
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
        std::cout << "checking num";
        SExpression* argument = eval(*expr.cdr->car);
        SExpression* result = numberPredicate(*argument);

        delete argument;

        return result;
    }

    return expr.clone();
}

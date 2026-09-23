#include "sexpression.h"

#include <stdexcept>
#include <iostream>

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

    output << " ";

    cdr->printList(output);
}

SExpression::~SExpression()
{
    delete car;
    delete cdr;
}

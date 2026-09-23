#ifndef SEXPRESSION_H
#define SEXPRESSION_H

#include "token.h"

#include <string>
#include <vector>
#include <ostream>

enum class ExpressionType
{
    Atom,
    Pair,
    Nil
};

class SExpression
{
public:
    ExpressionType type = ExpressionType::Nil;

    std::string atom;

    SExpression* car = nullptr;
    SExpression* cdr = nullptr;

    ~SExpression();

    void parse(const std::vector<Token>& tokens, size_t& position);
    void print(std::ostream &output);

private:
    void parseList(const std::vector<Token>& tokens, size_t& position);
    void printList(std::ostream &output);
};

#endif
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

    SExpression* clone() const;

    void parse(const std::vector<Token>& tokens, size_t& position);
    void print(std::ostream& output);

private:
    void parseList(const std::vector<Token>& tokens, size_t& position);
    void printList(std::ostream& output);

    bool isAtom() const;
    bool isNil() const;
    bool isPair() const;
};

SExpression* car(const SExpression& expr);
SExpression* cdr(const SExpression& expr);

SExpression* cons(const SExpression& first, const SExpression& second);

SExpression* quote(const SExpression& expr);

SExpression* eval(const SExpression& expr);

#endif
#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType
{
    Atom,
    LeftParen,
    RightParen
};

struct Token
{
    TokenType type;
    std::string value;
};

#endif

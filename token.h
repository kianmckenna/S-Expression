#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <vector>

enum class TokenType
{
    Atom,
    LeftParen,
    RightParen,
    Quote
};

class Token
{
public:

    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string &input);

#endif

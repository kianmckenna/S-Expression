#include "token.h"

#include <vector>
#include <string>
#include <cctype>

std::vector<Token> tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    std::string current;

    for (char c : input)
    {
        if (c == '\'')
        {
            if (!current.empty())
            {
                tokens.push_back({TokenType::Atom, current});
                current.clear();
            }

            tokens.push_back({TokenType::Quote, "\'"});
        }
        else if (c == '(')
        {
            if (!current.empty())
            {
                tokens.push_back({TokenType::Atom, current});
                current.clear();
            }

            tokens.push_back({TokenType::LeftParen, "("});
        }
        else if (c == ')')
        {
            if (!current.empty())
            {
                tokens.push_back({TokenType::Atom, current});
                current.clear();
            }

            tokens.push_back({TokenType::RightParen, ")"});
        } 
        else if (isspace(c))
        {
            if (!current.empty())
            {
                tokens.push_back({TokenType::Atom, current});
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
    {
        tokens.push_back({TokenType::Atom, current});
    }

    return tokens;
}

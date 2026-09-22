#include "sexpression.h"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <cctype>
#include <sstream>

using namespace std;

vector<Token> tokenize(const string &input);

int main()
{
    ifstream file("../input.txt");

    stringstream buffer;
    buffer << file.rdbuf();

    if (!file.is_open())
    {
        cout << "Error opening file" << endl;
        return 1;
    }

    string input = buffer.str();

    vector<Token> tokens = tokenize(input);

    file.close();

    size_t position = 0;

    while (position < tokens.size())
    {
        SExpression expr;
        expr.parse(tokens, position);
        expr.print();

        cout << endl;
    }

    return 0;
}

vector<Token> tokenize(const string &input)
{
    vector<Token> tokens;
    string current;

    for (char c : input)
    {
        if (c == '(')
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

#include "sexpression.h"
#include "token.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>

using namespace std;

int main()
{
    ifstream file("input.txt");

    stringstream buffer;
    buffer << file.rdbuf();

    string input = buffer.str();

    vector<Token> tokens = tokenize(input);

    file.close();

    size_t position = 0;

    ofstream output("output.txt");

    while (position < tokens.size())
    {
        SExpression expr;
        expr.parse(tokens, position);

        SExpression* result = eval(expr);

        result->print(output);
        output << '\n';

        delete result;
    }

    output.close();

    return 0;
}
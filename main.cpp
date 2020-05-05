#include <iostream>
#include "lexer.h"
using namespace std;

int main() {
    Lexer lexer("test.txt");
    
    if (lexer.IsFileOpen())
        cout << "file open successfully" << endl;
    else {
        cout << "file open failed" << endl;
        return 1;
    }

    int tok;
    while ((tok = lexer.GetTok()) != kTokEof)
        cout << tok << endl;
    
    return 0;
}

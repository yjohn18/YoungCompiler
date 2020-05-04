#include <string>
#include <fstream>
#include <cctype>
#include <iostream>

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
    kTokEof = -1,
    
    kTokDef = -2,
    kTokExtern = -3,
    
    kTokIdentifier = -4,
    kTokNumber = -5
};

class Lexer {
private:
    std::string identifier_str_;
    double num_val_;
    std::ifstream input_file_;
    int last_char_;
public:
    // 接收一个文件路径
    Lexer(std::string file_path);
    ~Lexer();
    bool IsFileOpen();
    int  GetTok();
};

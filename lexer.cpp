#include "lexer.h"

using namespace std;

// Lexer::Lexer(ifstream input_file) {
//     input_file_ = input_file;
//     // last_char_ = ' ';
// }

Lexer::Lexer(string file_path) {
    input_file_.open(file_path);
    last_char_ = ' ';
}

Lexer::~Lexer() {
    input_file_.close();
}

bool Lexer::IsFileOpen() {
    return input_file_.is_open();
}

int Lexer::GetTok() {

    // Skip any whitespace
    while (isspace(last_char_))
        last_char_ = input_file_.get();

    if (isalpha(last_char_)) {
        identifier_str_  = last_char_;
        while (isalnum(last_char_ = input_file_.get())) {
            identifier_str_ += last_char_;
        
        if (identifier_str_ == "def")
            return kTokDef;
        if (identifier_str_ == "extern")
            return kTokExtern;
        return kTokIdentifier;
    }

    if (isdigit(last_char_) || last_char_ == '.') {
        string num_str;
        do {
            num_str += last_char_;
            last_char_ = input_file_.get();
        } while (isdigit(last_char_) || last_char_ == '.');

        num_val_ = strtod(num_str.c_str(), nullptr);
        return kTokNumber;
    }

    if (last_char_ == '#') {
        do
            last_char_ = input_file_.get();
        while (last_char_ != EOF && last_char_ != '\n' && last_char_ != '\r');

        if (last_char_ != EOF)
            return GetTok();
    }

    if (last_char_ == EOF)
        return kTokEof;

    int this_char = last_char_;
    last_char_ = input_file_.get();

    return this_char;
}
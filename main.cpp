#include <iostream>
#include "parser.h"
#include "abstract_syntax_tree.h"

using namespace std;

int main() {
    Parser p("./test.txt");
    p.MainLoop();

    return 0;
}

#include <iostream>
#include "parser.h"

using namespace std;

int main() {
    Parser p("./test.txt");
    p.MainLoop();

    the_module_->print(llvm::errs(), nullptr);

    return 0;
}

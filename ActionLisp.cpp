#include "Interpreter.h"

int main(int argc, char* argv[])
{
    Interpreter inter;
    if (argc == 2) {
        inter.readFile(argv[1]);
    }
    else {
        inter.waitForInput();
    }
    return 0;
}

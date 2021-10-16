#include <iostream>
#include <string>

#include "../inc/assembler.hpp"

int main(int argc, char *argv[])
{

    bool no_errors;
    std::string SourceName, DestName;

    // Command: asembler -o izlaz.o ulaz.s

    // Handle command line parameters
    SourceName = argv[3];
    DestName = argv[2];

    // std::cout << SourceName << std::endl;
    // std::cout << DestName << std::endl;

    Assembler *AS = new Assembler(SourceName, DestName);
    no_errors = AS->assemble();

    if (!no_errors)
    {
        std::cout << std::endl
                  << "Assembly failed!" << std::endl;
    }
    else
    {
        std::cout << std::endl
                  << "Assembly successful!" << std::endl;
    }

    delete AS;

    return 0;
}
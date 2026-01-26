#include <complex>
#include <iostream>
// import aiger library - use extern C to disable name mangling
extern "C" {
    #include "aiger.h"
}

int main(int argc, char **argv) {
    // Parse .aig format using aiger library
    aiger* aig = aiger_init();

    const char* aiger_file =  aiger_open_and_read_from_file(aig, argv[1]);
    if (aiger_file != nullptr) {
        std::cerr << "Error opening file: " << aiger_file << '\n';
        aiger_reset(aig);
        return 1;
    }

    // Make sure we are correctly parsing the file
    std::cout << "Inputs: " << aig->num_inputs << '\n';
    std::cout << "Outputs: " << aig->num_outputs << '\n';
    std::cout << "ANDs: " << aig->num_ands << '\n';

    return 0;
}

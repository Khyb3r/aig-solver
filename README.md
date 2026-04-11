# Non-Clausal AIG SAT Solver
This is a Non-Clausal SAT Solver for the AIG representation.

## Folder Structure

- /src contains all Solver source code as well as other things explained below.
    - /Solver folder contains actual solver specific source code in the Solver.cpp and Solver.h files.
    - main.cpp (where the program starts to run from main())
    - aig.h (header file with the node structure of the AIG)
    - Arena.h (header file with the Arena Allocator code, all within the header file because it is templated)
    - aiger.c & aiger.h (THESE 2 FILES ARE THE ONLY PART THAT ARE NOT MY OWN CODE, THEY ARE TAKEN FROM THE AIGER LIBRARY BY ARMIN BIERE "https://github.com/arminbiere/aiger")
    - CMakeLists.txt build script (used to generate Makefile build scripts. Important for installing and running).
    - /cmake-build-release where the release build is and 'my_solver' binary is.
    - /cmake-build-debug where the debug build is and 'my_solver' binary is (kept here because CLion throws IDE error without it even when running the Release version).
    - /.idea folder with CLion related things (ignore this).

- /Benchmarks contains all three sets of benchmarks all with their corresponding '.aig' files.
    - /Bit-Blasted-SMTS and within it /smtqfbv-aigs containing the first set of benchmarks.
    - /Sequential-Unrolled and within it /tip-aig-20061215 containing the next set of benchmarks.
    - /Structural-SAT-All-UNSAT and within it /tip-k-ind-aigs-o1234g containing the final set of benchmarks (Has its own README by its author to see how it works).
    - /Unrolled-Outputs not a benchmark set but includes unrolled instances of the Sequential Circuits from /Sequential-Unrolled.

- /Scripts contains all the Python scripts for testing and experimentation.
    - single-run.py used for experimentation to run a single benchmark instance
    - group-run-and-log.py used to actually run a set on all solvers and record the results in /logs.

- /logs contains log folders for each run which have raw .log files within and a .json per run on a set of benchmarks.
    - Look at the naming of each subfolder within to see the actual json and raw .log files (naming should be self explanatory for what was being tested).

- /cadical-cnf-conversions contains '.cnf' files converted for their correspondong '.aig' files.


## Setup and Build (How to install and test)

### Requirements
Make sure you have the following installed: 
- A C++ Compiler that supports C++ 20.
- 'CMake' with at least Version 3.29
- 'Make' and ensure you can call it from your command line.
- At least Python 3.13.1.
- This project relies on the AIGER library.

#### The AIGER Library - AIGER has been self-contained into the project itself via 'aiger.c' and 'aiger.h' so there is no need to reinstall it
Note: If there are any issues go to "https://github.com/arminbiere/aiger" and look at how to install the library within its README.md and update the project structure by 
moving in the "aiger.c" and "aiger.h" files into the src/ folder so you can correctly use it.

## Building and Running
The Terminal Version is preferred.
### Terminal Version
Follow these instructions to compile the solver:
- Go to the project root 'aig-solver' folder: cd aig-solver
- Go to the 'src' folder: cd src
- Create a 'build' folder: mkdir build
- Go to the 'build' folder: cd build
- Call cmake here: cmake .. -DCMAKE_BUILD_TYPE=Release
- Call make here: make
- Now you have the 'my_solver' binary
Note: Cmake must be run from src/build directory

To run the solver on a benchmark instance, go to project root 'aig-solver': cd aig-solver
Go to the 'Scripts' folder: cd Scripts
Open up 'single-run.py'and edit the 'program' variable to have the path: "../src/build/./my_solver"
Save the Python file and run it with: python3 ./single-run.py

### IDE Version (CLion)
This is specific to CLion as it was used to develop the project, if you are using VSCode with C/C++ extensions or any other IDE
it is advised to use the Terminal Version above.
Steps:
- Open up the project within CLion.
- Go to CLion Settings -> Build, Execution, Deployment -> CMake -> Click on the '+' icon to add a new Profile
- Call it Release and set the Build Type to Release -> Apply -> Ok
- At the top right of CLion to the left of hammer and configuration tab there is a dropdown for which CMake Profile you want to set
- Set to Release -> Click Hammer icon to Build

To run the solver on a benchmark instance, go to project root 'aig-solver': cd aig-solver
Go to the 'Scripts' folder: cd Scripts
Run it with: python3 ./single-run.py
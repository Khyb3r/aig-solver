import os
import subprocess
from os import getcwd

def relative_path(*parts):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), *parts)

bit_blasted_SMTs_path = "Bit-Blasted-SMTs/smtqfbv-aigs"

# Has O1-05 and 0g
structural_SAT_All_UNSAT = "Structural-SAT-All-UNSAT/tip-k-ind-aigs-o1234g/O1"

preprocess_benchmark_path = structural_SAT_All_UNSAT
benchmarks_path = relative_path(f"../Benchmarks/{preprocess_benchmark_path}/")



file_extension = ".aig"
program = relative_path("../src/cmake-build-debug/./my_solver")
counter = 0
abc_path = os.path.realpath(relative_path("../../berkeley-abc/abc/abc"))

with os.scandir(benchmarks_path) as entries:
    for file in entries:
        if file.is_file() and file.name.endswith(file_extension):
            print(f"\nRunning: {file.name}")
            try:
                subprocess.run([program, file.path], check=True)

                abc_program = f'r {file.path}; sat'

                subprocess.run([abc_path, "-c", abc_program], check=True, cwd=os.path.dirname(abc_path))
            except subprocess.CalledProcessError as e:
                print(f"Error running {file.name}: {e}")
            counter += 1
            if counter >= 1:
                break

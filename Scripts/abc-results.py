import os
import subprocess

def relative_path(*parts):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), *parts)

#benchmark_file = relative_path("../Benchmarks/Structural-SAT-All-UNSAT/tip-k-ind-aigs-o1234g/O1/cmu.periodic.N.aig")
benchmark_file = relative_path("../Benchmarks/Bit-Blasted-SMTs/smtqfbv-aigs/bitops0.aig")
program        = relative_path("../src/cmake-build-debug/./my_solver")

abc_path = os.path.realpath(relative_path("../../berkeley-abc/abc/abc"))

print(f"Running: {os.path.basename(benchmark_file)}")

try:
    subprocess.run([program, benchmark_file], check=True)
except subprocess.CalledProcessError as e:
    print(f"Solver error: {e}")

try:
    abc_command = f'r {benchmark_file}; sat'
    subprocess.run([abc_path, "-c", abc_command], check=True, cwd=os.path.dirname(abc_path))
except subprocess.CalledProcessError as e:
    print(f"ABC error: {e}")
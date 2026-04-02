import os
import subprocess

def relative_path(*parts):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), *parts)

#benchmark_file = relative_path("../Benchmarks/Structural-SAT-All-UNSAT/tip-k-ind-aigs-o1234g/O4/texas.parsesys^2.E.aig")
benchmark_file = relative_path("../Benchmarks/Bit-Blasted-SMTs/smtqfbv-aigs/VS3-benchmark-S3.aig")
program        = relative_path("../src/cmake-build-debug/./my_solver")
logs_path = relative_path("../logs/Bit-Blasted-Runs/run_01/raw")
abc_path = os.path.realpath(relative_path("../../berkeley-abc/abc/abc"))
cadical_path = os.path.realpath(relative_path("../../cadical-lib/cadical/build"))
cadical_cnf_dir = relative_path("../cadical-cnf-conversions")
os.makedirs(cadical_cnf_dir, exist_ok=True)
cnf_file_for_cadical = os.path.join(cadical_cnf_dir, os.path.basename(benchmark_file) + ".cnf")

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

try:
    abc_command_cnf = f'r {benchmark_file}; write_cnf {cnf_file_for_cadical}'
    subprocess.run([abc_path, "-c", abc_command_cnf], check=True, cwd=os.path.dirname(abc_path))
except subprocess.CalledProcessError as e:
    print(f"ABC CNF generation error: {e}")
try:
    cadical_binary = os.path.join(cadical_path, "cadical")

    result = subprocess.run(
        [cadical_binary ,cnf_file_for_cadical],
        capture_output=True,
        text=True
    )

    print("CaDiCaL output:")
    print(result.stdout)

except subprocess.CalledProcessError as e:
    print(f"CaDiCaL error: {e}")

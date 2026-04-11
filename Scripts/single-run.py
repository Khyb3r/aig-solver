import os
import subprocess
import re

def relative_path(*parts):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), *parts)

# ONLY EDIT AROUND THIS AREA --------------------------------------------------------------------------------------------
# EDIT THIS TO CORRECT PATH DEPENDING ON THE SETUP YOU USE
program        = relative_path("../src/cmake-build-release/./my_solver")
# LOOK THROUGH BENCHMARKS IF YOU WANT TO TEST CHANGE THIS PATH TO THAT BENCHMARK (DO NOT PICK FROM SEQUENTIAL CIRCUITS)
benchmark_file = relative_path("../Benchmarks/Bit-Blasted-SMTs/smtqfbv-aigs/mult_ub_8x8_1.sf.aig")
# ------------------------------------------------------------------------------------------------------------------------


logs_path = relative_path("../logs/Bit-Blasted-Runs/run_01/raw")
abc_path = os.path.realpath(relative_path("../../berkeley-abc/abc/abc"))

cadical_path = os.path.realpath(relative_path("../../cadical-lib/cadical/build"))
cadical_cnf_dir = relative_path("../cadical-cnf-conversions")
os.makedirs(cadical_cnf_dir, exist_ok=True)
cnf_file_for_cadical = os.path.join(cadical_cnf_dir, os.path.basename(benchmark_file) + ".cnf")

output_dir = relative_path("../Benchmarks/Unrolled-Outputs")
unrolled_aig = os.path.join(output_dir, os.path.basename(benchmark_file))
os.makedirs(output_dir, exist_ok=True)
abc_command_unrolling = f""" r {benchmark_file}; strash; frames -F 7 -i ; orpos;  write_aiger -s {unrolled_aig}"""

"""
print(f"Running: {os.path.basename(benchmark_file)}")
try:
    subprocess.run([abc_path, "-c", abc_command_unrolling], check=True, cwd=os.path.dirname(abc_path))
except subprocess.CalledProcessError as e:
    print(f"ABC Unrolling error: {e}")
"""

# My Solver
try:
    subprocess.run([program, benchmark_file], check=True)
except subprocess.CalledProcessError as e:
    print(f"Solver error: {e}")


# MiniSAT
"""
try:
    abc_command = f'r {benchmark_file}; sat'
    subprocess.run([abc_path, "-c", abc_command], check=True, cwd=os.path.dirname(abc_path))
except subprocess.CalledProcessError as e:
    print(f"ABC error: {e}")


# CaDiCaL
try:
    abc_command_cnf = f'r {unrolled_aig}; write_cnf {cnf_file_for_cadical}'
    subprocess.run([abc_path, "-c", abc_command_cnf], check=True, cwd=os.path.dirname(abc_path))
except subprocess.CalledProcessError as e:
    print(f"ABC CNF generation error: {e}")


try:
    cadical_binary = os.path.join(cadical_path, "cadical")

    result = subprocess.run(
        [cadical_binary, cnf_file_for_cadical],
        capture_output=True,
        text=True
    )
    print("CaDiCaL output:")
    print(result.stdout)

    parsed = parse_cadical_output(result.stdout)
    print("\nParsed result:")
    print(parsed)

except subprocess.CalledProcessError as e:
    print(f"CaDiCaL error: {e}")
"""

import json
import os
import re
import subprocess

def relative_path(*parts):
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), *parts)

def parse_my_solver_output(output):
    data = {
        "benchmark": None,
        "result": None,
        "time": None
    }
    for line in output.splitlines():
        line = line.strip()

        if line.startswith("BENCHMARK"):
            data["benchmark"] = line.split(":", 1)[1].strip()
        elif line == "SAT" or line == "UNSAT":
            data["result"] = line
        elif line.startswith("TOTAL TIME:"):
            time_str = line.split(":", 1)[1].strip()
            data["time"] = float(time_str)
    return data

def parse_minisat_output(output):
    data = {
        "result": None,
        "time": None
    }
    for line in output.splitlines():
        line = line.strip()
        if (line.startswith("SATISFIABLE")):
            data["result"] = "SAT"
        elif (line.startswith("UNSATISFIABLE")):
            data["result"] = "UNSAT"
        match = re.search(r"Time\s*=\s*([0-9]*\.?[0-9]+)", line)
        if match:
            data["time"] = float(match.group(1))
    return data

def parse_preprocessing_output(output):
    data = {
        "total_inputs": None,
        "inputs_assigned": None,
        "true_forced": None,
        "false_forced": None,
        "ands_forced_percent": None
    }

    for line in output.splitlines():
        line = line.strip()

        if line.startswith("TOTAL INPUTS:"):
            data["total_inputs"] = int(line.split(":", 1)[1].strip())

        elif line.startswith("INPUTS ASSIGNED:"):
            data["inputs_assigned"] = int(line.split(":", 1)[1].strip())

        elif line.startswith("TRUE FORCED:"):
            data["true_forced"] = int(line.split(":", 1)[1].strip())

        elif line.startswith("FALSE FORCED:"):
            data["false_forced"] = int(line.split(":", 1)[1].strip())

        elif line.startswith("ANDS FORCED:"):
            data["ands_forced_percent"] = float(line.split(":", 1)[1].strip())

    return data

def parse_cadical_output(output):
    data = {
        "result": None,
        "time": None
    }
    for line in output.splitlines():
        line = line.strip()
        if (line.startswith("s SATISFIABLE")):
            data["result"] = "SAT"
        elif (line.startswith("s UNSATISFIABLE")):
            data["result"] = "UNSAT"
        elif(line.startswith("c total process")):
            match = re.search(r":\s*([0-9]*\.?[0-9]+)", line)
            if match:
                data["time"] = float(match.group(1))
    return data

#benchmarks_path = relative_path("../Benchmarks/Structural-SAT-All-UNSAT/tip-k-ind-aigs-o1234g/O1")
#benchmarks_path = relative_path("../Benchmarks/Bit-Blasted-SMTs/smtqfbv-aigs/")
benchmarks_path = relative_path("../Benchmarks/Sequential-Unrolled/tip-aig-20061215/")
my_solver_path = relative_path("../src/cmake-build-release/./my_solver")
abc_path = os.path.realpath(relative_path("../../berkeley-abc/abc/abc"))
cadical_path = os.path.realpath(relative_path("../../cadical-lib/cadical/build"))
cadical_cnf_conversions_path = relative_path("../cadical-cnf-conversions/")
os.makedirs(cadical_cnf_conversions_path, exist_ok=True)
unrolled_output_path = relative_path("../Benchmarks/Unrolled-Outputs/")

logs_base_path = relative_path("../logs/Sequential-Unrolled/")
logs_json_path = os.path.join(logs_base_path, "run.json")
logs_raw_path = os.path.join(logs_base_path, "raw")

os.makedirs(logs_raw_path, exist_ok=True)

run_data = {
    "run_id": "problem",
    "benchmarks": []
}

"""
with os.scandir(benchmarks_path) as entries:
    for file in entries:
        if file.is_file() and file.name.startswith("problem"):
            print(f"\nRunning: {file.name}")
            benchmark_entry = {
                "benchmark_file": file.name,
                "my_solver": {},
                "minisat": {},
                "cadical": {},
            }
            try:
                try:
                    solver_process = subprocess.run([my_solver_path, file.path], capture_output=True, text=True, check=True, timeout=15)
                    solver_output = solver_process.stdout
                    parsed_solver = parse_my_solver_output(solver_output)
                    benchmark_entry["my_solver"] = parsed_solver
                except subprocess.TimeoutExpired:
                    solver_output = ""
                    benchmark_entry["my_solver"] = {
                        "benchmark": None,
                        "result": None,
                        "time": None
                    }
                raw_log_file = os.path.join(logs_raw_path, f"{file.name}.log")

                try:
                    abc_command = f'r {file.path}; sat'
                    abc_process = subprocess.run([abc_path, "-c", abc_command], capture_output=True, text=True, cwd=os.path.dirname(abc_path), check=True, timeout=15)
                    abc_output = abc_process.stdout
                    parsed_abc = parse_minisat_output(abc_output)
                    benchmark_entry["minisat"] = parsed_abc
                except subprocess.TimeoutExpired:
                    parsed_abc = {
                        "result": None,
                        "time": None
                    }
                    benchmark_entry["minisat"] = parsed_abc
                    abc_output = ""

                try:
                    cnf_file_for_cadical = os.path.join(cadical_cnf_conversions_path, os.path.basename(file.path) + ".cnf")
                    abc_command_cnf = f'r {file.path}; write_cnf {cnf_file_for_cadical}'
                    subprocess.run([abc_path, "-c", abc_command_cnf], check=True, cwd=os.path.dirname(abc_path))
                    cadical_binary = os.path.join(cadical_path, "cadical")
                    cadical_process = subprocess.run(
                        [cadical_binary ,cnf_file_for_cadical],
                        capture_output=True,
                        text=True,
                        timeout=15
                    )
                    cadical_output = cadical_process.stdout
                    parsed_cadical = parse_cadical_output(cadical_output)
                    benchmark_entry["cadical"] = parsed_cadical
                except subprocess.TimeoutExpired:
                    benchmark_entry["cadical"] = {
                        "result": None,
                        "time": None
                    }
                    cadical_output = ""

                with open(raw_log_file, "w") as f:
                    f.write("=== MY SOLVER OUTPUT ===\n")
                    f.write(solver_output)
                    f.write("\n=== MINISAT OUTPUT ===\n")
                    f.write(abc_output)
                    f.write("\n=== CADICAL OUTPUT ===\n")
                    f.write(cadical_output)

            except subprocess.CalledProcessError as e:
                print(f"Error running {file.name}: {e}")
            run_data["benchmarks"].append(benchmark_entry)
with open(logs_json_path, "w") as j:
    json.dump(run_data, j, indent=4) """

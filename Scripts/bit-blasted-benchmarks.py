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



#benchmarks_path = relative_path("../Benchmarks/Structural-SAT-All-UNSAT/tip-k-ind-aigs-o1234g/O4")
benchmarks_path = relative_path("../Benchmarks/Bit-Blasted-SMTs/smtqfbv-aigs/")
my_solver_path = relative_path("../src/cmake-build-debug/./my_solver")
abc_path = os.path.realpath(relative_path("../../berkeley-abc/abc/abc"))

logs_base_path = relative_path("../logs/Bit-Blasted-Runs/run_03")
logs_json_path = os.path.join(logs_base_path, "run.json")
logs_raw_path = os.path.join(logs_base_path, "raw")

os.makedirs(logs_raw_path, exist_ok=True)

run_data = {
    "run_id": "run_03-phasesaving-andgateimportancescoring",
    "benchmarks": []
}

with os.scandir(benchmarks_path) as entries:
    for file in entries:
        if file.is_file() and file.name.startswith("convert-jpg2gif"):
            print(f"\nRunning: {file.name}")
            benchmark_entry = {
                "benchmark_file": file.name,
                "my_solver": {},
                "minisat": {}
            }
            try:
                try:
                    solver_process = subprocess.run([my_solver_path, file.path], capture_output=True, text=True, check=True, timeout=10)
                    solver_output = solver_process.stdout
                    parsed_solver = parse_my_solver_output(solver_output)
                    benchmark_entry["my_solver"] = parsed_solver
                except subprocess.TimeoutExpired:
                    parsed_solver = {
                        "benchmark": file.name,
                        "result": None,
                        "time": "N/A"
                    }
                    solver_output = ""
                    benchmark_entry["my_solver"] = parsed_solver
                raw_log_file = os.path.join(logs_raw_path, f"{file.name}.log")

                abc_command = f'r {file.path}; sat'
                try:
                    abc_process = subprocess.run([abc_path, "-c", abc_command], capture_output=True, text=True, cwd=os.path.dirname(abc_path), check=True, timeout=10)
                    abc_output = abc_process.stdout
                    parsed_abc = parse_minisat_output(abc_output)
                    benchmark_entry["minisat"] = parsed_abc
                except subprocess.TimeoutExpired:
                    parsed_abc = {
                        "result": None,
                        "time": "N/A"
                    }
                    benchmark_entry["minisat"] = parsed_abc
                    abc_output = ""
                with open(raw_log_file, "w") as f:
                    f.write("=== MY SOLVER OUTPUT ===\n")
                    f.write(solver_output)
                    f.write("\n\n=== ABC OUTPUT ===\n")
                    f.write(abc_output)
            except subprocess.CalledProcessError as e:
                print(f"Error running {file.name}: {e}")
            run_data["benchmarks"].append(benchmark_entry)
with open(logs_json_path, "w") as j:
    json.dump(run_data, j, indent=4)

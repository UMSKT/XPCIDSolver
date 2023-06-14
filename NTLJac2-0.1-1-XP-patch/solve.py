#!/usr/bin/env python
import os
import sys
import json
import subprocess
import time
import jsonschema
import select

SCHEMA = {
    "type": "object",
    "properties": {
        "name": {"type": "string"},
        "p": {"type": "string"},
        "x": {
            "type": "object",
            "properties": {
                "0": {"type": "string"},
                "1": {"type": "string"},
                "2": {"type": "string"},
                "3": {"type": "string"},
                "4": {"type": "string"},
                "5": {"type": "string"}
            },
            "required": ["0", "1", "2", "3", "4", "5"]
        },
        "pub": {"type": "string"},
        "ell": {"type": "array", "items": {"type": "integer"}},
        "s1p": {"type": "array", "items": {"type": "integer"}},
        "s2p": {"type": "array", "items": {"type": "integer"}}
    },
    "required": ["name", "p", "x", "pub", "ell", "s1p", "s2p"]
}


def main():
    start_time = time.time()

    if len(sys.argv) > 1 and sys.argv[1] == "-h":
        usage()

    with open(sys.argv[1], "r") as f:
        try:
            loaded_dict = json.load(f)
            jsonschema.validate(loaded_dict, SCHEMA)
            print("JSON Validation successful.")
            values = loaded_dict
            
        except jsonschema.exceptions.ValidationError as e:
            print(f"JSON Validation error: {e}")
            usage()

    os.makedirs(values["name"], exist_ok=True)
    log_file = open(f"{values['name']}/log.txt", "a")

    do_ell(values, filename=sys.argv[1], log_file=log_file)
    do_crt(values, filename=sys.argv[1], log_file=log_file)
    do_lmpmct(values, filename=sys.argv[1], log_file=log_file)
    do_inv_mod(values, filename=sys.argv[1], log_file=log_file)

    end_time = time.time()
    elapsed_time = end_time - start_time
    tee("Execution time:", elapsed_time, "seconds", file=log_file)


def usage():
    print(f"Usage: {sys.argv[0]} <inputfile.json> [options]")
    print("  -h : Display this help")
    exit(0)


def tee(*output, file=None, **kwargs):
    timestamp = time.strftime("[%H:%M:%S]", time.localtime())
    print(timestamp, *output, file=sys.stdout, **kwargs)

    if file is not None:
        print(timestamp, *output, file=file, **kwargs)


def update_json(data, filename, log_file=None):
    tee(f"Writing state to JSON {filename}")

    with open(filename, "w") as file:
        json.dump(data, file, indent=4)


def process_output_pipe(process=None, stdout=None, stderr=None, log_file=None):
    process_stdout = process.stdout
    process_stderr = process.stderr

    while True:
        try:
            ready_streams, _, _ = select.select([process_stdout, process_stderr], [], [])
        except ValueError:
            break

        for stream in ready_streams:
            if stream == process_stdout:
                stdout_line = stream.readline()
                if stdout_line:
                    stdout_line = stdout_line.strip()
                    tee(">", stdout_line, file=log_file)
                    stdout.append(stdout_line)
                else:
                    process_stdout.close()
            elif stream == process_stderr:
                stderr_line = stream.readline()
                if stderr_line:
                    stderr_line = stderr_line.strip()
                    tee("!", stderr_line, file=log_file)
                    stderr.append(stderr_line)
                else:
                    process_stderr.close()
        
        if process.poll() is not None and process_stdout.closed and process_stderr.closed:
            break


def do_ell(data, filename, log_file=None):
    ell = data.get("ell", [])
    s1p = data.get("s1p", [])
    s2p = data.get("s2p", [])

    if len(ell) != len(s1p) != len(s2p):
        print("JSON value lengths don't match! Refusing to process")
        usage()

    ell_todo = [5, 11, 13, 17, 19]
    curve = list(map(int, data["x"].values()))
    for i, ell_i in enumerate(ell_todo):
        if i < len(ell):
            tee(f"---------- Skipping order mod {ell_i} ----------\n", file=log_file)
            continue

        tee(f"---------- Solving order mod {ell_i} ----------\n", file=log_file)
        input_filename = f"{data['name']}/input_ell_{ell_i}.txt"

        with open(input_filename, "w") as f:
            f.write(str(data["p"]) + "\n")
            f.write(str(curve) + "\n")
            f.write(str(ell_i) + "\n")

        output_filename = f"{data['name']}/input_ell_{ell_i}_output.txt"

        process = subprocess.Popen(["./main", "-o", output_filename],
                                    stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE, universal_newlines=True)
                                   
        with open(input_filename, "r") as f:
            process.stdin.write(f.read())
            process.stdin.close()

        stdout = []
        stderr = []
        process_output_pipe(process=process, stdout=stdout, stderr=stderr, log_file=log_file)

        process.wait()

        with open(output_filename, "r") as f:
            res = f.read()

        ell_res = res.split(" ")
        ell.append(ell_i)
        s1p.append(int(ell_res[1]))
        s2p.append(int(ell_res[2]))

        data["ell"] = ell
        data["s1p"] = s1p
        data["s2p"] = s2p
        update_json(data, filename, log_file=log_file)


def do_crt(data, filename, log_file=None):
    crt_arr_ell = ",".join(map(str, data["ell"]))
    crt_arr_s1p = ",".join(map(str, data["s1p"]))
    crt_arr_s2p = ",".join(map(str, data["s2p"]))

    tee("---------- Calculating bigger modular information using CRT ----------\n", file=log_file)
    filename = f"{data['name']}/input_crt.txt"

    with open(filename, "w") as f:
        f.write(crt_arr_ell + "\n")
        f.write(crt_arr_s1p + "\n")
        f.write(crt_arr_s2p + "\n")

    process = subprocess.Popen(["./CRT", "-q"],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, universal_newlines=True)
                                
    with open(filename, "r") as f:
        process.stdin.write(f.read())
        process.stdin.close()

    stdout = []
    stderr = []
    process_output_pipe(process=process, stdout=stdout, stderr=stderr, log_file=log_file)

    process.wait()

    crt_res = stdout[-1].split(" ")
    crt_mod = int(crt_res[2])
    crt_s1p = int(crt_res[0])
    crt_s2p = int(crt_res[1])

    tee("CRT mod =", crt_mod, file=log_file)
    tee("CRT s1p =", crt_s1p, file=log_file)
    tee("CRT s2p =", crt_s2p, file=log_file)

    filename = f"{data['name']}/input_lmpmct.txt"

    with open(filename, "w") as f:
        f.write(str(data["p"]) + "\n")

        for value in list(data["x"].values())[:-1]:
            f.write(str(value) + "\n")

        f.write(str(crt_mod) + "\n")
        f.write(str(crt_s1p) + "\n")
        f.write(str(crt_s2p) + "\n")


def do_lmpmct(data, filename, log_file=None):
    tee("---------- Solving order from CRT results ----------\n", file=log_file)

    process = subprocess.Popen(["./LMPMCT", "-o", f"{data['name']}/output_lmpmct.txt"],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, universal_newlines=True)

    filename = f"{data['name']}/input_lmpmct.txt"
    
    with open(filename, "r") as f:
        process.stdin.write(f.read())
        process.stdin.close()

    stdout = []
    stderr = []
    process_output_pipe(process=process, stdout=stdout, stderr=stderr, log_file=log_file)

    process.wait()


def do_inv_mod(data, filename, log_file=None):
    with open(f"{data['name']}/output_lmpmct.txt", "r") as f:
        order = f.read()

    tee("---------- Calculating private key from order ----------\n", file=log_file)
    process = subprocess.Popen(["./InvMod", str(data['pub']), order],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, universal_newlines=True)

    stdout = []
    stderr = []
    process_output_pipe(process=process, stdout=stdout, stderr=stderr, log_file=log_file)

    process.wait()

    tee("Private key:", stdout[-1], "\n", file=log_file)
    tee("Decimal:", str(int(stdout[-1], 16)), "\n", file=log_file)


if __name__ == "__main__":
    main()

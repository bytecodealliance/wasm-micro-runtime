import subprocess

def get_func_name(func, file):
    cmd = ["wasm2wat", "--enable-all", file]
    grep_cmd = ["grep", "func"]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    grep_process = subprocess.Popen(
        grep_cmd, stdin=process.stdout, stdout=subprocess.PIPE
    )
    process.stdout.close()
    output = grep_process.communicate()[0].decode("utf-8")
    output = output.split("\n")
    output1 = [
        x
        for x in output
        if not x.__contains__("(type (;")
        # and not x.__contains__("(import ")
        and not x.__contains__("(table (;")
        and not x.__contains__("global.get")
    ]

    import_count = len([
        x
        for x in output
        if x.__contains__("(import ")
    ])

    for i in range(len(output1)):
        if i == func:
            return output1[i].split(" ")[3], i - import_count

if __name__ == "__main__":
    import sys
    wasm = sys.argv[1]
    names = []
    aot_idxes = []
    for i in range(2, len(sys.argv)):
        idx = int(sys.argv[i])
        name, aot_idx = get_func_name(idx, wasm)
        names.append(name)
        aot_idxes.append(aot_idx)
    for i in range(len(names)):
        print(f"{names[i]} {aot_idxes[i]}")
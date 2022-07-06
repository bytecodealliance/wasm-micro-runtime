#!/usr/bin/env python

import os
import pty
import re
import select
import shlex
import subprocess
import sys
import time

class TTYRunner:
    def __init__(self, cmd):
        self.process = None

        # Use tty to setup an interactive environment
        self.master_in, self.slave_in = pty.openpty()
        self.master_out, self.slave_out = pty.openpty()

        self.process = subprocess.Popen(
            cmd,
            bufsize=1,
            universal_newlines=True,
            stdin=self.slave_in,
            stdout=self.slave_out,
            stderr=subprocess.STDOUT,
        )
        self.buf = ""

    def read_to_prompt(self, prompts, timeout=5):
        begin = time.time()
        res = b""

        select_timeout = 2
        while time.time() - begin < timeout :
            outs, _, _ = select.select([self.master_out], [], [], select_timeout)
            if len(outs) == 0:
                print("[Runner WARNING]select timeout")
                break

            # first responding is slow, others are super fast
            select_timeout = 0.05

            data = os.read(self.master_out, 1024)
            if len(data) == 0:
                print("[Runner WARNING]just met an EOF")
                break

            print("[Runner WARNING]read {} bytes".format(len(data)))
            res += data

            if self.process.poll() is not None:
                print("[Runner WARNING]the process has quitted")
                break
        else:
            print("[Runner WARNING]after a long wait")

        print("[Runner INFO]got {} bytes, \"{}\"".format(len(res), res))

        # filter the prompts
        buf = None
        if len(res) > 0:
            self.buf = res.decode("utf-8")
            self.buf = self.buf.replace('\r\n', os.linesep)
            for prompt in prompts:
                match = re.search(prompt, self.buf)
                if match:
                    end = match.end()
                    buf = self.buf[0 : (end - len(prompt))]
                    self.buf = self.buf[end:]
                    print("[Runner WARNING]find a match")
                    break
            else:
                print("[Runner WARNING]doesn't find a match")

            print("[Runner INFO]  buf is \"{}\"".format(buf))
            print("[Runner INFO]  self.buf is \"{}\"".format(self.buf))
        return buf

    def writeline(self, str):
        if self.process.poll() is not None:
            return

        str_to_write = str + "\n"
        str_to_write = str_to_write.encode("utf-8")
        os.write(self.master_in, str_to_write)

    def cleanup(self):
        if self.process.poll() is None:
            try:
                # _, _ = self.process.communicate()
                # self.writeline("__exit__")

                self.process.terminate()
                self.process.communicate(timeout=0.3)
                self.process.wait(timeout=0.3)
            except Exception as e:
                print("process shutdown time-out {}".format(e))
                self.process.kill()
        else:
            print("[Runner INFO]process has already quitted")

        for fd in [self.slave_in, self.slave_out, self.master_in, self.master_out]:
            os.close(fd)

        return self.process.returncode

if __name__ == "__main__":
    runner = TTYRunner(shlex.split("ls /"))
    runner.read_to_prompt([" usr"])
    runner.cleanup()

    runner = TTYRunner(shlex.split("python3"))
    runner.read_to_prompt([">>> "])
    runner.writeline("3 + b")
    runner.read_to_prompt([">>> "])
    runner.writeline("3 + 2")
    runner.read_to_prompt([">>> "])
    runner.writeline("exit()")
    runner.read_to_prompt([">>> "])
    runner.cleanup()

    runner = TTYRunner(shlex.split("python3"))
    runner.read_to_prompt([">>> "])
    runner.cleanup()

    runner = TTYRunner(shlex.split("python3"))
    runner.read_to_prompt([">>>>>> "])
    runner.cleanup()

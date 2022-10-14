#!/usr/bin/env python3

import os

file_list = []
# build riscv-tests to this folder
BUILD_DIR = "../riscv-tests/build/isa"
DST_DIR = "./tests"
TEST_PRIFIX = ["rv64ui-p-","rv64um-p-","rv64ua-p-","rv64mi-p-"]
for (dirpath, dirnames, filenames) in os.walk(BUILD_DIR):
    for x in filenames:
        if x.endswith(".dump"):
            continue
        for y in TEST_PRIFIX:
            if x.startswith(y):
                file_list.append(x)
file_list.sort()

def make_test():
    os.system("mkdir -p {}".format(DST_DIR))
    for x in file_list:
        os.system("riscv64-unknown-linux-gnu-objcopy -O binary {}/{} {}/{}.bin".format(BUILD_DIR,x,DST_DIR,x))

def run_all():
    for x in file_list:
        print("Testing {}: ".format(x),end="",flush=True)
        os.system("./cemu {}/{}.bin -rvtest".format(DST_DIR,x))

make_test()
run_all()
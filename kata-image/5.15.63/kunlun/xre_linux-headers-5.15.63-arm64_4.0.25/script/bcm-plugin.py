#!/usr/bin/env python
# encoding: utf-8

"""
\brief Plugin for BCM monitor service
\authors hanjinchen@baidu.com
"""

import shlex
import subprocess
import sys
from collections import namedtuple

def __run_cmd(binary, args, background=False):
    """execute given command, if run in bg, return popen handler, otherwise return retval and output"""
    if binary is None:
        raise RuntimeError("binary is none!")

    cmd = "{} {}".format(binary, args)
    #print "run cmd '{}'".format(cmd)

    popen = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if background:
        return 0, popen

    popen.wait()
    output = popen.stdout.read()
    return popen.returncode, output


def __main():
    ret, _ = __run_cmd("which", "xpu_smi")
    if ret != 0:
        sys.exit(1)

    ret, out = __run_cmd("xpu_smi", "-m")
    if ret != 0:
        sys.exit(1)

    tokenized_lines = [line.split() for line in out.strip().split('\n')]

    for ltk in tokenized_lines:
        def __oout(key, value, is_str=False):
            """Print one monitering item"""
            if is_str:
                print "Xpu{}{}:'{}'".format(ltk[2], key, value)
            else:
                print "Xpu{}{}:{}".format(ltk[2], key, value)

        __oout("PCIAddr", ltk[0], True)
        __oout("BoardId", ltk[1])
        __oout("DevId", ltk[2])
        __oout("TempPD0", ltk[3])
        __oout("TempPD1", ltk[4])
        __oout("TempHBM0", ltk[5])
        __oout("TempHBM1", ltk[6])
        __oout("Power", ltk[7])
        __oout("Freq0", ltk[8])
        __oout("Freq1", ltk[9])
        __oout("Freq2", ltk[10])
        __oout("Freq3", ltk[11])
        __oout("Freq4", ltk[12])
        __oout("Freq5", ltk[13])
        __oout("L3Used", ltk[14])
        __oout("L3Size", ltk[15])
        __oout("MemUsed", ltk[16])
        __oout("MemUsed", ltk[17])
        __oout("UseRatio", ltk[18])
        __oout("FirmVersion", ltk[19], True)


if __name__ == "__main__":
    __main()

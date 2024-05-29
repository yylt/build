#!/bin/bash

set -x

#/tmp/mlnx/mlnxofedinstall -k 5.15.63 --add-kernel-support
#perl -MCPAN -e 'install CPAN'
#perl -MCPAN -e 'install File::Find'

/tmp/MLNX_OFED_LINUX-5.8-1.1.2.1-ubuntu22.04-aarch64/mlnxofedinstall -k 5.15.63 --add-kernel-support
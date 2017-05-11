#!/bin/bash
VMID= $(create -c 1 -m 0x1000000 -s 0x200000 -n mac=0,dev=eth0,ibuf=1024,obuf=1024,iband=1000000000,oband=1000000000,pool=0x400000
upload $VMID main
start $VMID

# PacketNgin system image guide

Semih Kim (semih@gurum.cc)
2015-11-20

---

## Table of contents
[TOC]

## Disk partitioning

|-------------|----------------|--------------|---------------------|
| Location    | Name           | Filesystem   | Description         |
|=============|================|==============|=====================|
| 0MB ~ 1MB   | BIOS Boot      | (bootloader) | Grub MBR and Core   |
|-------------|----------------|--------------|---------------------|
| 1MB ~ 2MB   | Boot partition | VFAT         | PacketNgin Loader   |
|-------------|----------------|--------------|---------------------|
| 2MB ~ 16MB  | Root partition | BFS          | PacketNgin Kernel   |
|-------------|----------------|--------------|---------------------|
| 16MB ~ 64MB | User partition | ext2         | User disk image     |
|-------------|----------------|--------------|---------------------|


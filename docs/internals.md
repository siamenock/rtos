# PacketNgin Internals

Semih Kim (semih@gurum.cc)
2014-05-30

---

# Table of contents
[TOC]

# Overall Architecture

## Boot
* 16 bites, 512 bytes code block
* Boot loads loader to memory and executes it

## Loader
* 32 bits kernel loader
* Loads 64 bites O/S kernel

## Kernel
* 64 bits O/S kernel
* Wake up core 1 to n
* Executes manager on core 0

## Manager
* O/S controller
* Have console and web interface

## App
* User application running on PacketNgin O/S
* Compiled with PacketNgin SDK's library


# Booting Sequence

## BIOS loads and executes MBR
1. BIOS(Basic Input Output System) reads MBR(Master Boot Record)
1. BIOS loads MBR code block(512 bytes) to memory
1. BIOS executes the block

## Boot loads and executes Loader
1. Boot gathers system memory map
1. Boot loads Loader to 0x100000 (1MB)
1. Boot executes Loader

## Loader copys and executes Kernel
1. Loader set GDT(General Descriptor Table)
1. Loader activates A20 gate
1. Loader change the CPU mode from real mode(16 bits) to protected mode(32 bits)
1. Loader copy the Kernel to 0x200000 (2MB)
1. Loader initialize page tables
1. Loader change the CPU model from protected mode(32 bits) to long mode(64 bits)

## Kernel on BP(Boot Processor) boot APs(Application Processors)
1. Kernel jump from 0x200000 to 0xffffffff80200000 using virtual memory
1. Kernel initialize shared area(b/w kernels)
1. Kernel initialize malloc area
1. Kernel initialize global malloc area
1. Kernel analyize CPU information
1. Kernel activate multi-core
1. Kernel initialize GDT again
1. Kernel initialize TSS(Task State Segment)
1. Kernel initialize IDT(Interrupt Descriptor Table)
1. Kernel initialize PCI devices
1. Kernel initialize APIC(Advanced Programmable Interrupt Controller)
1. Kernel initialize I/O APIC
1. Kernel activate multi-tasking facility
1. Kernel initailize inter-core communication
1. Kernel loads kernel symbols
1. Kernel loads modules
1. Kernel initialize network facility
1. Kernel initialize events
1. Kernel clean up unnecessery memory
1. Kernel initialize device drivers
1. Kernel initialize network interfaces
1. Kernel executes Manager and Shell

## Manager and shell waits user's command
1. Manager waits user's command from web
1. Shell waits user's command from console


# Kernel Components
* main - Manage kernel components
* Standard I/O - Manage console input and output
* Shared memory - Shared memory between cores
* Local memory - Core's local memory
* Gloal memory - Globally accessable memory between cores (generally not shared)
* CPU - CPU initialization, getting CPU clock
* Multi-Processor - Multi-processor initialization, analyzation, and synchronization
* GDT - Manage GDT
* TSS - Manage TSS
* IDT - Manage IDT
* PCI - Analyize and initialize PCI devices
* APIC - Analyzize and initialize APIC and manage interrupt
* Multi-Task - Multi-tasking in a core
* ICC - Inter-Core Communications using message passing and Extra-core interrupts
* Kernel symbol map - Manage kernel symbols (used by device drivers)
* Module - Load and relocate module
* Device Driver - Manage device drivers
* Network Interface - Manage multiple network interfaces (physical and virtual)
* Event - Manage kernel events (one time event, timed event, idle time event, ....)
* Manager - Web console
* Shell - Text console
* Device driver interfaces
    * Character Input
    * Character Output
    * Network Interface Card
* Device drivers
    * VGA
    * Keyboard
    * Pseudo Standard Output
    * Pseudo Standard Input

# Inter-Core Communications
* Start VM - Start user process
* Pause VM - Puase user process
* Stop VM - Stop user process

# Web Control Framework
* Create VM - Create a virtual machine
* Delete VM - Destroy the virtual machine
* Start VM - Boot the virtual machine
* Stop VM - Suspend the virtual machine
* Get VM power status - Get virtual machine's power status (started, or stopped)
* Upload VM storage - Upload VM's disk image
* Download VM storage - Download VM's disk image
* Delete VM storage - Clean VM's disk image
* Standard input/output/error - Input and output VM's standard I/O/E

# SDK Components
* Newlib - Standard C library
* Core - Core library
* LWIP - Light Weight Internet Protocol
* JSMN - JSON parser
* zlib - Compression library
* OpenSSL - Cryptography library

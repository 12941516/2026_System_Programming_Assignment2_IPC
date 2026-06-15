# Simple-IPC

Simple-IPC is a Linux kernel module assignment skeleton.

## Directory Layout

```text
simple-ipc/
├── apps/
│   ├── server.c
│   └── client.c
├── include/
│   ├── abi.h
│   └── kv.h
├── kernel/
│   ├── Makefile
│   └── core.c
├── docs/
│   └── README.md
└── Makefile
```

## Installation

Build the project:

```bash
make
```

Load the kernel module:

```bash
sudo insmod .build/simipc.ko
```

Verify that the module is loaded:

```bash
sudo dmesg | tail
```

Unload the kernel module:

```bash
sudo rmmod simipc
```

Clean build artifacts:

```bash
make clean
```
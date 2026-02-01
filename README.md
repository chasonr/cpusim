# cpusim
This program presents an emulated CPU and some memory. It provides windows to show disassembly, registers and contents of memory.

The current version provides an emulated NMOS 6502. Undocumented opcodes are not supported; this is planned as a separate setting. Other planned features include 65C02 and Z80.

## Build requirements

I am building this on Linux Mint with GCC 13.3 and wxWidgets 3.2.4. The Makefile is very simple and likely adaptable to other Linux configurations.

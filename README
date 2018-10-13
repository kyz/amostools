amostools - tools to work with AMOS programs

* dumpamos: extract the contents of AMOS source code and memory banks
* listamos: list AMOS source code as plain text
* unlockamos: unlock "locked" procedures in AMOS source code

USAGE
=====

Extract banks attached to source code:
$ dumpamos source.amos

Extract multi-banks to single banks:
$ dumpamos multi.abs

Extract sprites or icons as IFF ILBM images:
$ dumpamos sprites.abk

Use input filenames as a prefix for the output filenames:
$ dumpamos -p source.amos multi.abs sprites.abk

List normal AMOS programs:
$ listamos source.amos

List AMOS programs that use 3rd-party extensions:
$ listamos -e extensions/CRAFT.Lib-V1.00 \
           -e extensions/MusiCRAFT.Lib-V1.00 \
           source.amos

List AMOS programs that use extensions in non-standard slots:
$ listamos -e23=extensions/Dump.Lib-V1.0 \
           source.amos      # ...or with extensions in non-standard slots

List AMOS programs using the extensions configured in your own AMOS setup:
$ listamos -c myamos/AMOS1_3_Pal.Env -d myamos/AMOS_System source.amos
$ listamos -c myamos/AMOS1_3_Pal.Env -d myamos/AMOS_System source.amos
$ listamos -c myamos/s/AMOSPro_Interpreter_Config -d myamos/APSystem source.amos

Unlock AMOS programs with locked procedures:
$ unlockamos Fold.Acc *.AMOS

ABOUT EXTENSIONS
================

AMOS supports extensions to its core language. It provides four standard
extensions (Music, Compact, Requester, Serial/IOPorts), but many other
extensions exist.

Extensions must be loaded into a "slot" to use them. There are 25
slots.  When you use extension instructions, AMOS only stores the slot
number and a token table offset in your source code.

So, to list programs written using an extension, you need that exact
extension, and it needs to be loaded in the correct slot.

listamos has the four standard extensions built in, and for your
convenience, the extensions/ directory contains "stripped" copies
of all extensions I can find - they contains only the extensions'
instruction names, and none of the extensions' code.

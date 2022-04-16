amostools - tools to work with AMOS programs

* dumpamos: extract the contents of AMOS source code and memory banks
* listamos: list AMOS source code as plain text
* unlockamos: unlock "locked" procedures in AMOS source code

Usage
=====

Extract banks attached to source code:

    $ dumpamos source.amos

Extract sprites, pictures, samples from source or banks:
    
    $ dumpamos source.amos sprites.abk pic1.abk samps.abk

Use input filenames as a prefix for the output filenames:

     $ dumpamos -p source.amos

List AMOS programs:

    $ listamos source.amos

List AMOS programs that use 3rd-party extensions:

    $ listamos -e extensions/CRAFT.Lib-V1.00 \
               -e extensions/MusiCRAFT.Lib-V1.00 \
               source.amos
    
    $ listamos -e23=extensions/Dump.Lib-V1.0 \
               source.amos   # you can load extensions in non-standard slots

List AMOS programs using the extensions configured in your own AMOS setup:

    $ listamos -c myamos/AMOS1_3_Pal.Env -d myamos/AMOS_System source.amos
    $ listamos -c myamos/AMOS1_3_Ntsc.Env -d myamos/AMOS_System source.amos
    $ listamos -c myamos/s/AMOSPro_Interpreter_Config -d myamos/APSystem source.amos

Unlock AMOS programs with locked procedures:

    $ unlockamos Fold.Acc *.AMOS

About Extensions
================

AMOS supports [extensions](https://www.exotica.org.uk/wiki/AMOS_extensions) to its core language.

AMOS provides four standard extensions (Music, Compact, Requester, Serial/IOPorts),
but many other extensions exist.

Extensions must be loaded into a "slot" to use them. There are 25
slots.  When you use extension instructions, AMOS only stores the slot
number and a token table offset in your source code.

So, to list programs written using an extension, you need that exact
extension, and it needs to be loaded in the correct slot.

`listamos` has the four standard extensions built in, and for your
convenience, the `extensions/` directory contains as many extensions
as I can find.

Further resources
=================

* [AMOS File Formats](https://www.exotica.org.uk/wiki/AMOS_file_formats)
* [AMOS Extensions](https://www.exotica.org.uk/wiki/AMOS_extensions)
* [AMOS Documentation](https://gitlab.com/amigasourcecodepreservation/amos-classic-documentation)
* Get [AMOS 1.34](https://amr.abime.net/issue_198_coverdisk), [AMOS 1.35 and AMOS Compiler](https://amr.abime.net/issue_602_coverdisks), [AMOS 3D](https://amr.abime.net/issue_505_coverdisks), [Easy AMOS](https://amr.abime.net/issue_530_coverdisks)
* Get [AMOS Pro 1.11](https://amr.abime.net/issue_223_coverdisks), [AMOS Pro 1.0 and 2.0](http://www.classicamiga.com/content/view/5027/175/)
* [AMOS 1.x source code](https://web.archive.org/web/20071130053321/http://clickteam.com/eng/downloadcenter.php?i=58)
* [AMOS Pro source code](https://github.com/AMOSFactory/AMOSProfessional)


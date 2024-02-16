# cpu6502 based system capable of running Woz Monitor

This directory contains the orignal Woz Monitor code for the Apple I computer (<tt>wozmon.asm</tt>) and top level code to instantiate the cpu6502 processor model with a simple PIA model that has memory mapped registers between <tt>0xD0010</tt> and <tt>0xD0013</tt> for keyboard input and character display output, to match that expected for the source code.

A <tt>makefile</tt> is used to compile the cpu6502 library, the main top level code, and assemble wozman. To compile and run the code use <tt>make run</tt>. This has been tested on Ubuntu Linux and MSYS2/mingw-64 with the <tt>gcc</tt> toolchain. By default, the [CC65](https://www.cc65.org/) suite is used for assembling and linking, and is assumed to be on the path.

When built, a standalone executable, <tt>main.exe</tt>, is generated aong with a <tt>cpu6502.bin</tt> binary containing Wozman. To run from the command line directly use, assumimg built from <tt>make</tt>:

    main.exe

This assumes that the file <tt>cpu6502.bin</tt> has been built as a 32Kbyte image for loading at address <tt>0x8000</tt> and that the image will populate the reset vector at <tt>0xFFFA</tt>. Other command line options exists to change the filename and other parameters. The usage message is as shown below:

    Usage: main.exe [-f <filename>][-l <addr>][-t <program type>][n][-d]
    
        -t Program format type                 (default BIN)
        -f program file name                   (default cpu6502.[ihex|bin] depending on format)
        -l Load start address of binary image  (default 0x8000)
        -r Reset vector address                (default set from program)
        -n Disable line feed generation        (default false)
        -d Enable disassembly                  (default false)

The format type (</tt>-t</tt> option) is either HEX or BIN for Intel hex format or binary files. The default program can be overridden with <tt>-f</tt>, where the default name will be either <tt>cpu6502.ihex</tt> or <tt>cpu6502.bin</tt>, depending on the type (default HEX). The load address (<tt>-l</tt> option) can be overridden for binary files (Intel Hex files ignore this parameter) and the reset vector value updated (<tt>-r</tt> option) to jump to a given location on reset. The <tt>-n</tt> option disables generating linefeed on carriage return characters, and the <tt>-d</tt> option enable generation of disassembly output from the cpu6502 model.

## Credits
Derived from the Ben Eater (@beneater) [project](https://github.com/beneater/msbasic). See also Ben Eater's [YouTube video](https://www.youtube.com/watch?v=7M8LvMtdcgY).
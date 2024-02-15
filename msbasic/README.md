# Microsoft Basic ported for the cpu6502 model

This is a version of Microsft Basic that has been ported to the cpu6502 ISS. It uses the model from the <tt>wozmon</tt> directory and the <tt>makefile</tt> in this directory can be used to compile basic and the mode (type <tt>make</tt>). This generates a <tt>main.exe</tt> executable. To run MSBASIC on the model you can use <tt>make run</tt>. Alternatively the executable can be run directly with the following command line options:


    main.exe -n -tBIN -l 0x8000 -r 0x8000


The usage message (use the <tt>-h</tt> option) for the executable is:

    Usage: main.exe [-f <filename>][-l <addr>][-t <program type>][n][-d]
    
        -t Program format type                 (default HEX)
        -f program file name                   (default cpu6502.[ihex|bin] depending on format)
        -l Load start address of binary image  (default 0x8000)
        -r Reset vector address                (default 0xff00)
        -n Disable line feed generation        (default false)
        -d Enable disassembly                  (default false)

The format type is either HEX or BIN for Intel hex format or binary files. The default program can be overridden with <tt>-f</tt>, where the default name will be either <tt>cpu6502.ihex</tt> or <tt>cpu6502.bin</tt>, depending on the type (default HEX). The load address (<tt>-l</tt> option) can be overridden for binary files (Intel Hex files ignore this parameter) and the reset vector value updated (<tt>-r</tt> option) to jump to a given location on reset. The <tt>-n</tt> option disables generating linefeed on carriage return characters, and the <tt>-d</tt> option enable generation of disassembly output from the cpu6502 model.

When run, you will be asked for a memory size (try 53248&mdash;equivalent to a memtop of 0xD000, the page where the PIA model is mapped). A value of 0 usually starts an automatic RAM size discovery, but this is not working in the cpu6502 model at this time. You will then be asked for a terminal width, and you can just press enter for this. You will then be in MSBASIC. Note that a running basic program can be interrupted with &lt;ESC&gt; and the model executable exited with ^C.

## Credits
Derived from the Ben Eater (@beneater) [project](https://github.com/beneater/msbasic)
which was forked from the Michael Steil (@mist64) [project](https://github.com/mist64/msbasic). See also Ben Eater's [YouTube video](https://www.youtube.com/watch?v=XlbPnihCM0E).

# Microsoft Basic ported for the cpu6502 model

This is a version of Microsft Basic that has been ported to the cpu6502 ISS. It uses the model from the <tt>wozmon</tt> directory and the <tt>makefile</tt> in this directory can be used to compile basic and the model (type <tt>make</tt>). The assembling and linking of the MS Basic code uses the [CC65](https://github.com/cc65/cc65) tools which must be downloaded and compiled locally and the resultant <tt>bin/</tt> directory placed in the <tt>PATH</tt>. Building with make generates a <tt>main.exe</tt> executable of the mdoel and a <tt>cpu6502.bin</tt> binary file with the compiled MS Bssic. To run MSBASIC on the model you can use <tt>make run</tt>. Alternatively the executable can be run directly with the following command line options (to suppress linefeed generation):


    main.exe -n


The usage message (use the <tt>-h</tt> option) for the executable is:

    Usage: main.exe [-f <filename>][-l <addr>][-t <program type>][n][-d]
    
        -t Program format type                 (default BIN)
        -f program file name                   (default cpu6502.[ihex|bin] depending on format)
        -l Load start address of binary image  (default 0x8000)
        -r Reset vector address                (default set from program)
        -n Disable line feed generation        (default false)
        -d Enable disassembly                  (default false)

The format type is either HEX or BIN for Intel hex format or binary files. The default program can be overridden with <tt>-f</tt>, where the default name will be either <tt>cpu6502.ihex</tt> or <tt>cpu6502.bin</tt>, depending on the type (default BIN). The load address (<tt>-l</tt> option) can be overridden for binary files (Intel Hex files ignore this parameter) and the reset vector value updated (<tt>-r</tt> option) to jump to a given location on reset. The <tt>-n</tt> option disables generating linefeed on carriage return characters, and the <tt>-d</tt> option enable generation of disassembly output from the cpu6502 model.

When run, you will be asked for a memory size, but hitting return without a value will instigate an auto detection of RAM. You will then be asked for a terminal width, and you can just press enter for this as well. You will then be in MSBASIC. Note that a running basic program can be interrupted with &lt;ESC&gt; and the model executable exited with ^C.

## Credits
Derived from the Ben Eater (@beneater) [project](https://github.com/beneater/msbasic)
which was forked from the Michael Steil (@mist64) [project](https://github.com/mist64/msbasic). See also Ben Eater's [YouTube video](https://www.youtube.com/watch?v=XlbPnihCM0E).

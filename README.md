# cpu6502: A 6502 Instruction Set Simulator written in C++.

Contains a C++ model of the MOS 6502 microprocessor with compilation options to configure the model, and an optional command line interface. A test program (modified) from Klaus Dormann (https://github.com/Klaus2m5/6502_65C02_functional_tests) is also present to test the instructions' validity. A simple API is provided for integration with other models or applications&mdash;see the manual in <tt>doc/user_guide.pdf</tt> and the header file <tt>src/cpu6502_api.h</tt> for more information. Support is provided for either building under MSVC 2010 (cpu6502.sln) or under Linux (makefile).

The model has been integrated and tested with <a href="http://www.mkw.me.uk/beebem">BeebEm</a> (v4.14) as part of its validation, with the simple steps needed documented in the manual.
As well as being integrated with BeebEm, a port of the APPLE I system monitor (aka WozMon, written by Steve Wozniac) is provide and will run on an example model incorporating cpu6502 (see <tt>wozmon</tt> directory), and Microsoft Basic is also supported on this same model  (see <tt>msbasic</tt> directory). Both use the [CC65](https://github.com/cc65/cc65) toolchain to assemble and link the code, and makefiles in each directory are provided to build and run the model and the assembled programs under Linux or under Windows with MSYS2/mingw-w64 (<tt>make run</tt>).

simon@anita-simulators.org.uk

www.anita-simulators.org.uk/wyvernsemi

<p>&nbsp;</p>
<p align="center">
<img style="box-shadow: 7px 7px 7px #a0a0a0;" src="https://github.com/wyvernSemi/cpu6502/assets/21970031/19da40af-11b0-4adb-b6c1-026ba3eca6c0">
</p>
<p>&nbsp;</p>


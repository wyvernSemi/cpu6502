# cpu6502
A 6502 Instruction Set Simulator written in C++. The model has been integrated and tested with <a href="http://www.mkw.me.uk/beebem">BeebEm</a>.
<p>&nbsp;</p>
<p align="center">
<img src="https://github.com/wyvernSemi/cpu6502/assets/21970031/19da40af-11b0-4adb-b6c1-026ba3eca6c0">
</p>
<p>&nbsp;</p>

Includes a C++ model of the MOS 6502, with compile options, and an optional command line interface. A test program (modified) from Klaus Dormann (https://github.com/Klaus2m5/6502_65C02_functional_tests) is included. An API is provided for integration with other applications&mdash;see the manual in <tt>doc/user_guide.pdf</tt> and the header file <tt>src/cpu6502_api.h</tt> for more information.

Support is provided for building under MSVC 2010 (cpu6502.sln) and Linux (makefile).

simon@anita-simulators.org.uk

www.anita-simulators.org.uk/wyvernsemi

# IEEE754
Task #1 of the class Advanced Programming 2020.

Soft implementation of IEEE 754 with basic expression parser.

For some task submitting reason, it is a single-file program.

## Build

Just use `gcc` or `clang` to compile the file `float64.c` with option `-O2`. SSE2 must be supported.

## Usage

It reads one line with an expression which can only include regular numbers, `+`, `-`, `*`, `/` and brackets from `stdin`. All spaces are filtered within the line.  
Then the program evaluates the expression and prints the value with a precision of 1200 digits after the decimal point (the exact value of the float number).

NOTE: Buffer size is `100010` bytes long and the expression given to the program must be valid.

### **SIC Assembler Project**

This project contains two different implementations of an assembler for the SIC architecture. One version completes the assembly in a single scan of the source code, while the other uses the traditional two-scan approach.





#### **Files in this folder**

onepass.c (the single pass assembler code)

twopass.c (the two pass assembler code)

README.md (readme)

opcodes.txt

sample_input.txt




#### **How to compile the programs**

If you have 'make' installed on your system, you can just type make.

This will create two executable files named 'onepass' and 'twopass'. 

If you want to remove the executables and output files later, you can type

make clean





#### **How to run the programs**

Before running, make sure opcodes.txt and sample\_input.txt are in the same folder as the compiled programs.

To run the one pass assembler

./onepass

To run the two pass assembler

./twopass





#### **How the logic works**

The one pass assembler is a bit more complex because it has to handle labels that haven't been defined yet. It does this by keeping a linked list of memory locations that need an address. When the label is finally defined, the program goes back to its memory buffer and patches the correct address into the instructions.

The two pass assembler is more straightforward. In the first pass, it just calculates the addresses of every label and stores them in a symbol table. In the second pass, it reads the file again and uses that table to write the final object code.



#### 

#### **Output files**

After running the programs, you will see new files in your folder.

output\_onepass.txt (the result from the one pass assembler)

output\_twopass.txt (the result from the two pass assembler)

intermediate.txt (a temporary file created by the two pass version)

Both assemblers are designed to follow the standard SIC record format, including the H (Header), T (Text), and E (End) records. They also handle indexed addressing and ensure that instructions are never split across two different text records.



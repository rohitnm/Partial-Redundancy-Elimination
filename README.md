-----------------------------------------------------------
Readme for Final Project of ECE5544 Compiler Optimization
Group: Rohit Mehta, Shambhavi Kuthe
-----------------------------------------------------------
Instructions:
Extract the zip file in llvm-project/llvm/lib/Transforms

Follow the instructions given below:

cd Transforms
cd <zip-folder>
cd PRE
make clean
make
-----------------------------------------------------------
TO TEST THE LCM PASS
-----------------------------------------------------------

make run-test1      //To test final PRE via LCM pass for test1.c file
make run-test2      //To test final PRE via LCM pass for test2.c file
make run-test3      //To test final PRE via LCM pass for test3.c file

------------------------------------------------------------
TO TEST INDIVIDUAL PASS
------------------------------------------------------------
make run-preprocessor
make run-anticipated
make run-available
make run-postponable
make run-used

------------------------------------------------------------

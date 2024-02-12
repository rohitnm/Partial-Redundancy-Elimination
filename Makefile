INC=-I/usr/local/include/
all: anticipated.so available.so postponable.so used.so preprocessor.so lcm.so
CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) $(INC) -g -O0 -fPIC

anticipated.o: anticipated.cpp anticipated.h
available.o: available.cpp available.h
postponable.o: postponable.cpp postponable.h
used.o: used.cpp used.h
preprocessor.o: preprocessor.cpp
lcm.o: lcm.cpp
dataflow.o: dataflow.cpp dataflow.h

%.so: %.o dataflow.o
	$(CXX) -dylib -shared $^ -o $@

run-test1: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test1.c -o ./tests/test1.bc
	opt -mem2reg ./tests/test1.bc -o ./tests/test1_m2r.bc
	llvm-dis ./tests/test1_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test1_m2r.bc -o ./tests/test1_mod.bc
	llvm-dis ./tests/test1_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -load ./available.so -load ./postponable.so -load ./used.so -load ./lcm.so -lazy-code-motion ./tests/test1_mod.bc -o ./tests/test1_out.bc
	llvm-dis ./tests/test1_out.bc

run-test2: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test2.c -o ./tests/test2.bc
	opt -mem2reg ./tests/test2.bc -o ./tests/test2_m2r.bc
	llvm-dis ./tests/test2_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test2_m2r.bc -o ./tests/test2_mod.bc
	llvm-dis ./tests/test2_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -load ./available.so -load ./postponable.so -load ./used.so -load ./lcm.so -lazy-code-motion ./tests/test2_mod.bc -o ./tests/test2_out.bc
	llvm-dis ./tests/test2_out.bc

run-test3: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test3.c -o ./tests/test3.bc
	opt -mem2reg ./tests/test3.bc -o ./tests/test3_m2r.bc
	llvm-dis ./tests/test3_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test3_m2r.bc -o ./tests/test3_mod.bc
	llvm-dis ./tests/test3_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -load ./available.so -load ./postponable.so -load ./used.so -load ./lcm.so -lazy-code-motion ./tests/test3_mod.bc -o ./tests/test3_out.bc
	llvm-dis ./tests/test3_out.bc

run-preprocessor: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test1.c -o ./tests/test1.bc
	opt -mem2reg ./tests/test1.bc -o ./tests/test1_m2r.bc
	llvm-dis ./tests/test1_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test1_m2r.bc -o ./tests/test1_mod.bc
	llvm-dis ./tests/test1_mod.bc

run-anticipated: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test1.c -o ./tests/test1.bc
	opt -mem2reg ./tests/test1.bc -o ./tests/test1_m2r.bc
	llvm-dis ./tests/test1_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test1_m2r.bc -o ./tests/test1_mod.bc
	llvm-dis ./tests/test1_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -anticipated ./tests/test1_mod.bc -o ./tests/test1_out.bc
	llvm-dis ./tests/test1_out.bc

run-available: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test1.c -o ./tests/test1.bc
	opt -mem2reg ./tests/test1.bc -o ./tests/test1_m2r.bc
	llvm-dis ./tests/test1_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test1_m2r.bc -o ./tests/test1_mod.bc
	llvm-dis ./tests/test1_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -load ./available.so -available ./tests/test1_mod.bc -o ./tests/test1_out.bc
	llvm-dis ./tests/test1_out.bc

run-postponable: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test1.c -o ./tests/test1.bc
	opt -mem2reg ./tests/test1.bc -o ./tests/test1_m2r.bc
	llvm-dis ./tests/test1_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test1_m2r.bc -o ./tests/test1_mod.bc
	llvm-dis ./tests/test1_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -load ./available.so -load ./postponable.so -Postponable ./tests/test1_mod.bc -o ./tests/test1_out.bc
	llvm-dis ./tests/test1_out.bc

run-used: all
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c ./tests/test1.c -o ./tests/test1.bc
	opt -mem2reg ./tests/test1.bc -o ./tests/test1_m2r.bc
	llvm-dis ./tests/test1_m2r.bc
	opt -enable-new-pm=0 -load ./preprocessor.so -preprocessor ./tests/test1_m2r.bc -o ./tests/test1_mod.bc
	llvm-dis ./tests/test1_mod.bc
	opt -enable-new-pm=0 -load ./anticipated.so -load ./available.so -load ./postponable.so -load ./used.so -Used ./tests/test1_mod.bc -o ./tests/test1_out.bc
	llvm-dis ./tests/test1_out.bc

clean:
	rm -f *.o *~ *.so

.PHONY: clean all

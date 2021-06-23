LLVMPREFIX=${LLVMBASEDIR}/llvm-4.0/bin
LLVMGOLD_LINK=""
LLVMGOLD_CC="${LLVMGOLD_PREFIX}/bin/clang -flto -O0"
LLVMGOLD_CXX="${LLVMGOLD_PREFIX}/bin/clang++ -flto -O0"
LLVMGOLD_LINKFLAGS="-O0 -g3 -Wl,-plugin-opt=-disable-fp-elim -Wl,-plugin-opt=save-temps"
LLVMGOLD_OPTFLAGS="-disable-inlining -disable-opt"
CFLAGS+="-DLLVM_HAS_MPX=1 -fPIC"
LDFLAGS+=" -fPIC"

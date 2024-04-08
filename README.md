# Assignment1_G23
Primo assignment di Linguaggi e compilatori gruppo 23 - Ronchetti(164907), Pini(169105).
## Procedura di utilizzo
Copiare i file nelle rispettive directory:
* LocalOpts.h -> SRC_ROOT/llvm/include/llvm/Transforms/Utils
* PassRegistry.def e PassBuilder.cpp -> SRC_ROOT/llvm/lib/Passes
* LocalOpts.cpp e CMakeLists.txt-> SRC_ROOT/llvm/lib/Transforms/Utils/
  In seguito si può ricompilare opt nella cartella BUILD col seguente comando:\
  <code>make opt</code>\
  Dopo aver ricompilato è possibile andare nella root directory di LLVM(quella in cui sono presenti le directory BUILD,TEST,SRC,INSTALL) ed eseguire i seguenti comandi:\
  <code>export PATH=/root/LLVM_ROOT/INSTALL/bin:$PATH\
  BUILD/bin/opt -p localopts TEST/Foo.ll -o TEST/Foo.optimized.bc\
  cd TEST
  llvm-dis Foo.optimized.bc -o Foo.opt.ll </code>

# Assignment3_G23
Terzo assignment di Linguaggi e compilatori gruppo 23 - Ronchetti(164907), Pini(169105).
## Contenuto della cartella
La cartella contiente tutti i file di configurazione per poter utilizzare il passo con anche un file per verificare il funzionamento.

## Procedura di utilizzo
Copiare i seguenti file nelle rispettive directory:
- LICM.c -> file opzionale per poter visualizzare il codice in C e da cui viene creato LOOP.ll tramite il passo di mem2reg usando opt e llvm-dis
- LOOP.ll -> <code>TEST/</code>
- LoopInvariant.h -> <code>SRC_ROOT/llvm/include/llvm/Transforms/Utils</code>
- PassRegistry.def e PassBuilder.cpp -> <code>SRC_ROOT/llvm/lib/Passes</code>
- LoopInvariant.cpp e CMakeLists.txt-> <code>SRC_ROOT/llvm/lib/Transforms/Utils/</code>\
  <br>
  In seguito si può ricompilare opt nella cartella BUILD col seguente comando <code>make opt</code>\
  <br>
  Dopo aver ricompilato è possibile andare nella root directory di LLVM(quella in cui sono presenti le directory BUILD,TEST,SRC,INSTALL) ed eseguire i seguenti comandi:\

```
  export PATH=/root/LLVM_ROOT/INSTALL/bin:$PATH\
  BUILD/bin/opt -p loopinvariant TEST/LOOP.ll -o TEST/FINAL.bc\
  cd TEST
  llvm-dis FINAL.bc -o FINAL.ll
```


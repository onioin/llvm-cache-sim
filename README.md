# An llvm Cache Simulator
This project implements an llvm pass, which, when run on a bitcode file, injects a cache simulator.

## Building
To build, run

```commandline
export LLVM_DIR={install directory for llvm}
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR -B{build directory} -S{project source directory}
cd {build directory}
make
```

## Running
### To run using the tool
```commandline
cd {build directory}/bin
./inject-cache-sim <input bitcode file> [cache options] -o<output binary file> -l<libraries to link> -L<directories to search>
```
### To run using opt
```commandline
export BUILD_DIR={project build directory}
opt --load-pass-plugin=$BUILD_DIR/lib/libFindMemCall.so --load $BUILD_DIR/lib/libInjectCacheSim.so --load-pass-plugin=$BUILD_DIR/lib/libInjectCacheSim.so --passes="inject-cache-sim" [cache options] <input bitcode file> -o<output bitcode file>
clang <output bitcode file> -L$BUILD_DIR/cache-sim/ -lcache-sim -o<output binary file>
```
## Options
```commandline
Cache Options
-E <num>         - Number of lines per set (associativity/number of ways)
-b <num>         - Number of bits used for byte offset (block size is 2^b bytes)
--name=<string>  - Name of the cache (optional)
--policy=<value> - Set the cache replacement policy:
  =lfu           -   Least Frequently Used
  =lru           -   Least Recently Used
  =fifo          -   First In First Out
  =rand          -   Random
-s <num>         - Number of bits used for set index (number of sets is 2^s)

Tool Options
-o <filename>    - Output filename
-O <level>       - Clang optimization level (default -O2)
-l <library>     - Specify libraries to link against
-L <directory>   - Specify library search paths
```
## Acknowledgements
This project uses code from [llvm-tutor](https://github.com/banach-space/llvm-tutor/tree/main)
and [llvm-demo](https://github.com/nsumner/llvm-demo/tree/master)

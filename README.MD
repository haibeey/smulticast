# smulticast 
multicast over tcp (https://x.com/haibbeey/status/2103773948186177901?s=20)

To push this idea further , one can do this in kernel space (L2 network) with something like eBPF.

```
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake -S . -B build -DBUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON # build the test
cmake --build build  
./build/smulticast example/config.ini
./build/smulticast_test # Run the test
```

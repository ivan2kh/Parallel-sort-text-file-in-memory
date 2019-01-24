## Parallel sort text file in memory

The program allocates memory to fit the complete input file.
Furthermore extra memory required for supplement structures: about 12 bytes per line.

When the file is loaded we calculate **Sorting Integer Key** for each line. 
**SIK** is a **Big Endian** integer value representing beginning of the line.

Default [Thread Building Blocks](https://www.threadingbuildingblocks.org/) sort algorithm used to sort lines.
The line comparison operator employ _integer_ _key_ if possible
 or switch to regular string comparison in worst case.

### Build requirements

* **GCC 7 or Clang 3.9**. There are some C++17 features used in code so you will need a modern compiler
* **Thread Building Blocks**. Use ```apt install libtbb-dev```

### Build

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Running

Input file name must be passed as first argument. The result will be in STDOUT.
```
./string_sort input_file > sorted_file
```

### Important notes
In order to achieve a better sorting performance the only ASCII character set is supported.

But due to implementation specifics some *UTF-8* texts will be sorted decently too.
At least _new_ _line_ character always parsed correctly in [UTF-8](https://en.wikipedia.org/wiki/UTF-8#Description).

Also sorting result differs from Linux's sort one. For example
```
1234567
123456
```
Will be converted to
```
123456
1234567
```
This is default C++ strings comparison behaviour.  
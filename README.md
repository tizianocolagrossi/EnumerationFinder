# EnumerationFinder

## Requirements
- libclang-13
- clang-13
- llvm-13

```bash
apt install clang-13 libclang-13-dev llvm-13-dev 
```

## How to build
```bash
cmake -B ./build -S .
cd ./build/
make

or 

cmake -B ./build -S . && cd ./build/ && make
```

the finders will be placed inside `build/src/`

## How to use
```bash
export CC="build/src/finder"
export CXX="build/src/finder++"

export REALCC="clang"
export REALCXX="clang++" 
```
By using the finder executables like a compiler will output a file named `EnumerationFinder.dump` that will contain all the information of the enumeration found in the sourcecode.

You can filter the output enumeration by name using the environment variable `SHOW_ONLY`.

## Output example
```text
EnumDecl with name: Weekdays at line4:6 in file: ./test.c
@ ./test.c
4: enum Weekdays {
5:     MONDAY,
6:     TUESDAY,
7:     WEDNESDAY,
8:     THURSDAY,
9:     FRIDAY,
10:     SATURDAY,
11:     SUNDAY
12: };

Variable Found
@ ./test.c
16:     enum Weekdays today;
16:                   ^
Used
@ ./test.c
19:     today = TUESDAY;
19:     ^
Used
@ ./test.c
22:     switch (today) {
22:   
```
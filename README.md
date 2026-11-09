# pionpi

`pionpi` is a C++17 command-line program that uses the local
[Apfloat](C:\src\apfloat) library to calculate Pi to millions of decimal
digits. It evaluates the Chudnovsky series with exact signed-integer binary
splitting, then converts the final ratio to guarded Apfloat precision.

## Build

```powershell
cmake -S . -B build -DAPFLOAT_SOURCE_DIR=C:\src\apfloat
cmake --build build --config Release
```

The default `APFLOAT_SOURCE_DIR` is `C:/src/apfloat`. If Apfloat has been
installed as a CMake package instead, set that cache variable to a nonexistent
path and configure with `CMAKE_PREFIX_PATH` pointing to its install prefix.

## Use

The default run writes one million fractional digits to standard output:

```powershell
.\build\Release\pionpi.exe --output pi-1000000.txt
```

Choose another precision with `--digits`:

```powershell
.\build\Release\pionpi.exe --digits 10000
```

`--output FILE` writes the result to a file; otherwise it is written to
standard output. The result is rounded to the requested number of fractional
decimal digits.

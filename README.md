# pionpi

`pionpi` is a C++17 command-line program that calculates Pi to millions of
decimal digits. It evaluates the Chudnovsky series with exact signed-integer
binary splitting, then converts the final ratio to guarded Apfloat precision.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

On its first configure, CMake downloads Apfloat from GitHub at the pinned
commit required by Pionpi. Git and network access are therefore required for
that initial configure; subsequent builds reuse CMake's populated build tree.

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

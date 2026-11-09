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

## Raspberry Pi 5

Pionpi supports a 64-bit Raspberry Pi OS/Debian Linux system on a Raspberry Pi
5 with 8 GB RAM. The **Raspberry Pi** GitHub Actions workflow is manually
dispatched on the default branch and requires a self-hosted runner with the
`self-hosted`, `Linux`, `ARM64`, `raspberry-pi`, and `pionpi` labels.

An Apfloat runner is registered to the Apfloat repository and cannot process
Pionpi jobs. The existing runner account and prerequisites may be reused, but
register a separate Pionpi runner. From a readable Pionpi checkout on the Pi,
as the normal runner user:

```sh
./scripts/pi-setup.sh
./scripts/pi-session.sh
```

Setup requires an interactive terminal, 64-bit ARM Debian-based Linux, CMake
3.20 or newer, a C++17 compiler, Git, Curl, JQ, and at least 2 GiB free in the
runner user's home filesystem. It downloads the official ARM64 GitHub Actions
runner to `~/.local/share/pionpi/actions-runner`, verifies its published
SHA-256 digest, and prompts for a short-lived repository registration token.
Do not put that token in commands, logs, or repository files.

Start the runner only for a validation session, manually dispatch the workflow,
and stop the foreground runner when it completes. The workflow builds Release,
runs the 10 through 100,000-digit vector tests, and runs the one-million-digit
extended test. Keep the runner offline when unused: public-repository
self-hosted runners execute matching repository workflow jobs and require
careful repository-administration controls.

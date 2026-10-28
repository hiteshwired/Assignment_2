# STM32 Matrix Keypad Scanner

A bare-metal embedded C exercise for the STM32L476RG (Nucleo-64) that scans a
4&times;3 matrix keypad and displays the pressed key's value as a binary pattern
on three LEDs. GPIO is configured at the register level.

## What it does

- Configures `PC0`&ndash;`PC2` as outputs (3-bit LED display) and the keypad rows
  (`PC4`&ndash;`PC7`) and columns (`PC8`&ndash;`PC10`) as a scanned matrix.
- Reads the keypad by driving columns and sensing rows, mapping the active
  row/column to a key via a lookup table (`1`&ndash;`9`, `*`, `0`, `#`).
- Shows the detected key on the LEDs.

## Hardware

- STM32L476RG (Nucleo-64)
- 4&times;3 matrix keypad on `PC4`&ndash;`PC10`, 3 LEDs on `PC0`&ndash;`PC2`

## Build

Generated from STM32CubeMX with a CMake build. Build with the included CMake
presets and an `arm-none-eabi` toolchain, or open in STM32CubeIDE, then flash.

---

## Academic honesty & licensing

Coursework from an embedded systems course at Cal Poly, San Luis Obispo. The
`main.c` application logic is my own work. The project also bundles
STMicroelectronics' STM32L4xx HAL/CMSIS drivers and CubeMX-generated files, which
are under ST's own license terms and are **not** mine to relicense.

Shared as a learning reference only. It is intentionally not released under an
open-source license, and you should not submit it as your own work for a graded
assignment.

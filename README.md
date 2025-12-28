# O3SerialWriter

Lightweight Arduino logging library for consistent Serial output. Adds optional prefix, millis timestamp, log levels with filtering, and type-safe variadic logging (C# params style) so you can write `sw.info("Backoff", backoff, "ms")` without repetitive `Serial.print` chains.

## Features

- Consistent log line format
- Optional prefix (example: `NET`)
- Optional `millis()` timestamp
- Log levels: Debug, Info, Warn, Error
- Minimum log level filtering
- Variadic logging with any number of parts
- No dynamic memory allocation, prints directly to `Stream`

## Installation

### Option A, Arduino library folder

Create this structure in your Arduino sketchbook libraries folder:

```
O3SerialWriter/
  library.properties
  src/
    O3SerialWriter.h
```

Restart Arduino IDE.

### Option B, Git submodule or copy

Copy the `O3SerialWriter` folder into your sketchbook `libraries` folder, or add as a git submodule there.

## Quick start

```cpp
#include <O3SerialWriter.h>

O3SerialWriter sw;

void setup() {
  O3SerialWriterOptions options;
  options.prefix = "NET";
  options.showMillis = true;
  options.showLevel = true;
  options.minLevel = O3LogLevel::Debug;
  options.partSeparator = " ";

  sw.begin(Serial, 115200, options);

  sw.info("Boot");
}

void loop() {
  int backoff = 250;
  int attempt = 3;
  int statusCode = 503;

  sw.info("Backoff", backoff, "ms");
  sw.warn("HTTP", statusCode, "retry in", backoff, "ms", "attempt", attempt);

  delay(1000);
}
```

Example output:

```
[NET] 1234 INFO: Boot
[NET] 2234 INFO: Backoff 250 ms
[NET] 3234 WARN: HTTP 503 retry in 250 ms attempt 3
```

## Configuration

`O3SerialWriterOptions`:

- `prefix`: text printed at the beginning of every line
- `showMillis`: prints `millis()` when true
- `showLevel`: prints `DEBUG/INFO/WARN/ERROR` when true
- `minLevel`: filters out logs below this level
- `partSeparator`: string printed between variadic parts

Example:

```cpp
O3SerialWriterOptions options;
options.prefix = "BLE";
options.minLevel = O3LogLevel::Warn;
options.partSeparator = " | ";

sw.begin(Serial, 115200, options);

sw.debug("This will not print");
sw.warn("Disconnected", "reason", 19);
```

## Working with multiple `print()` calls

If you want to manually build a line, the header is printed only once per line:

```cpp
sw.printWithLevel(O3LogLevel::Info, "Backoff ");
sw.printWithLevel(O3LogLevel::Info, 250);
sw.printlnWithLevel(O3LogLevel::Info, " ms");
```

## License
MIT.

#pragma once

#include <Arduino.h>

// Log levels, ordered from least important to most important.
// minLevel works like a filter, for example if minLevel = Warn, then Debug and Info are skipped.
enum class O3LogLevel : uint8_t {
  Debug = 0,
  Info  = 1,
  Warn  = 2,
  Error = 3,
  None  = 255
};

// Configuration options passed once during setup, you can also reconfigure later.
struct O3SerialWriterOptions {
  const char* prefix = "";                 // Printed at the beginning of every log line, for example "NET"
  bool showMillis = true;                  // If true, prints millis() in each line
  bool showLevel = true;                   // If true, prints INFO/WARN/...
  O3LogLevel minLevel = O3LogLevel::Debug; // Minimum level to print
  const char* partSeparator = " ";         // Separator between parts in variadic logs
};

class O3SerialWriter {
public:
  O3SerialWriter() = default;

  // Convenience: initialize Serial with baud, then configure the writer.
  // Example: sw.begin(Serial, 115200, options);
  void begin(HardwareSerial& serialPort, uint32_t baud, const O3SerialWriterOptions& options = O3SerialWriterOptions()) {
    serialPort.begin(baud);
    begin(static_cast<Stream&>(serialPort), options);
  }

  // Generic begin: lets you log to any Stream (Serial, Serial1, WiFiClient, etc.).
  void begin(Stream& stream, const O3SerialWriterOptions& options = O3SerialWriterOptions()) {
    out = &stream;
    configure(options);
  }

  // Apply options without re-opening the serial port.
  void configure(const O3SerialWriterOptions& options) {
    copyPrefix(options.prefix);
    copyPartSeparator(options.partSeparator);
    showMillis = options.showMillis;
    showLevel = options.showLevel;
    minLevel = options.minLevel;
    resetLineState();
  }

  // Small setters for runtime changes.
  void setPrefix(const char* newPrefix) { copyPrefix(newPrefix); }
  void setMinLevel(O3LogLevel level) { minLevel = level; }
  void setPartSeparator(const char* value) { copyPartSeparator(value); }

  // Enable/disable all logging at once.
  void setEnabled(bool isEnabled) {
    enabled = isEnabled;
    // If we disable mid-line, reset internal state so we do not keep a "half-open" line.
    if (!enabled) resetLineState();
  }

  bool isEnabled() const { return enabled; }

  // ---------------------------------------------------------------------------
  // Low-level print/println (defaultLevel = Info)
  // These are useful when you want to manually build a line with multiple calls.
  // The header is printed only once per line.
  // ---------------------------------------------------------------------------

  template <typename T>
  void print(const T& value) {
    printWithLevel(defaultLevel, value);
  }

  template <typename T>
  void println(const T& value) {
    printlnWithLevel(defaultLevel, value);
  }

  void println() {
    if (!canWrite(defaultLevel)) return;
    ensureLineHeader(defaultLevel);
    out->println();
    resetLineState();
  }

  template <typename T>
  void printWithLevel(O3LogLevel level, const T& value) {
    if (!canWrite(level)) return;
    ensureLineHeader(level);
    out->print(value);
  }

  template <typename T>
  void printlnWithLevel(O3LogLevel level, const T& value) {
    if (!canWrite(level)) return;
    ensureLineHeader(level);
    out->println(value);
    resetLineState();
  }

  // ---------------------------------------------------------------------------
  // High-level log API
  //
  // Overload #1: single message (const char*)
  // Overload #2: variadic template (First, Rest...) which allows 2+ parts
  //
  // Example:
  //   sw.info("Boot");
  //   sw.info("Backoff", backoff, "ms");                  // 3 parts
  //   sw.warn("HTTP", statusCode, "retry in", backoff);   // 4 parts
  //
  // In C++, "variadic templates" are the type-safe equivalent of C# params.
  // ---------------------------------------------------------------------------

  void debug(const char* message) { line(O3LogLevel::Debug, message); }
  void info(const char* message)  { line(O3LogLevel::Info,  message); }
  void warn(const char* message)  { line(O3LogLevel::Warn,  message); }
  void error(const char* message) { line(O3LogLevel::Error, message); }

  // Variadic overloads: accept any number of parts (2 or more) and print them separated.
  // Signature explanation:
  //   First is the first argument type, Rest... are the remaining argument types.
  //   const First& means "pass by reference", avoids copies for bigger types.
  template <typename First, typename... Rest>
  void debug(const First& first, const Rest&... rest) { parts(O3LogLevel::Debug, first, rest...); }

  template <typename First, typename... Rest>
  void info(const First& first, const Rest&... rest) { parts(O3LogLevel::Info, first, rest...); }

  template <typename First, typename... Rest>
  void warn(const First& first, const Rest&... rest) { parts(O3LogLevel::Warn, first, rest...); }

  template <typename First, typename... Rest>
  void error(const First& first, const Rest&... rest) { parts(O3LogLevel::Error, first, rest...); }

private:
  // Stream is Arduino's generic "thing you can print to" base type.
  // Serial and WiFi clients derive from Stream/Print.
  Stream* out = nullptr;

  bool enabled = true;
  bool showMillis = true;
  bool showLevel = true;

  O3LogLevel minLevel = O3LogLevel::Debug;
  O3LogLevel defaultLevel = O3LogLevel::Info;

  // These track whether we already printed the header for the current line.
  bool lineOpen = false;
  O3LogLevel activeLevel = O3LogLevel::Info;

  // Fixed-size buffers (no heap allocation).
  // Arduino-friendly: avoid dynamic allocation to reduce memory fragmentation.
  static constexpr size_t prefixMaxLen = 32;
  char prefixBuffer[prefixMaxLen] = "";

  static constexpr size_t partSepMaxLen = 8;
  char partSeparatorBuffer[partSepMaxLen] = " ";

  // Safe copy into fixed-size buffer (always null-terminated).
  void copyPrefix(const char* value) {
    if (!value) value = "";
    size_t i = 0;
    for (; i + 1 < prefixMaxLen && value[i] != '\0'; i++) {
      prefixBuffer[i] = value[i];
    }
    prefixBuffer[i] = '\0';
  }

  void copyPartSeparator(const char* value) {
    if (!value || value[0] == '\0') value = " ";
    size_t i = 0;
    for (; i + 1 < partSepMaxLen && value[i] != '\0'; i++) {
      partSeparatorBuffer[i] = value[i];
    }
    partSeparatorBuffer[i] = '\0';
  }

  void resetLineState() {
    lineOpen = false;
    activeLevel = defaultLevel;
  }

  // Central filter logic: if this returns false, we do not print anything.
  bool canWrite(O3LogLevel level) const {
    if (!enabled) return false;
    if (!out) return false;
    if (minLevel == O3LogLevel::None) return false;
    return static_cast<uint8_t>(level) >= static_cast<uint8_t>(minLevel);
  }

  const char* levelText(O3LogLevel level) const {
    switch (level) {
      case O3LogLevel::Debug: return "DEBUG";
      case O3LogLevel::Info:  return "INFO";
      case O3LogLevel::Warn:  return "WARN";
      case O3LogLevel::Error: return "ERROR";
      default:                return "LOG";
    }
  }

  // Writes header once per line:
  // Example output:
  //   [NET] 12345 INFO: <your message here>
  void writeHeader(O3LogLevel level) {
    if (prefixBuffer[0] != '\0') {
      out->print('[');
      out->print(prefixBuffer);
      out->print("] ");
    }

    if (showMillis) {
      out->print(millis());
      out->print(' ');
    }

    if (showLevel) {
      out->print(levelText(level));
      out->print(": ");
    }
  }

  // Ensures the header is printed exactly once for a given line.
  // If we were already building a line and the caller changes log level,
  // we finish the old line and start a new one.
  void ensureLineHeader(O3LogLevel level) {
    if (!lineOpen) {
      activeLevel = level;
      writeHeader(level);
      lineOpen = true;
      return;
    }

    if (level != activeLevel) {
      out->println();
      resetLineState();
      activeLevel = level;
      writeHeader(level);
      lineOpen = true;
    }
  }

  // Single message printing.
  void line(O3LogLevel level, const char* message) {
    if (!canWrite(level)) return;
    ensureLineHeader(level);
    out->println(message);
    resetLineState();
  }

  // Print one "part". Arduino's Stream::print supports many types (const char*, int, long, float, etc.).
  template <typename T>
  void writePart(const T& value) {
    out->print(value);
  }

  // Recursive variadic printer:
  // - prints the first part
  // - if there are remaining parts, prints separator then prints the rest
  //
  // if constexpr is a compile-time if. It only compiles the branch that is needed.
  template <typename First, typename... Rest>
  void writeParts(const First& first, const Rest&... rest) {
    writePart(first);
    if constexpr (sizeof...(rest) > 0) {
      out->print(partSeparatorBuffer);
      writeParts(rest...);
    }
  }

  // Variadic log: prints all parts in one line.
  template <typename First, typename... Rest>
  void parts(O3LogLevel level, const First& first, const Rest&... rest) {
    if (!canWrite(level)) return;
    ensureLineHeader(level);
    writeParts(first, rest...);
    out->println();
    resetLineState();
  }
};

#include <O3SerialWriter.h>

O3SerialWriter sw;

void setup() {
  O3SerialWriterOptions options;
  options.prefix = "NET";
  options.showMillis = true;
  options.showLevel = true;
  options.minLevel = O3LogLevel::Debug;
  options.partSeparator = " ";
  options.lineLength = 32;
  options.lineCharacter = '=';

  sw.begin(Serial, 115200, options);

  sw.info("Boot");
  sw.drawLine();
}

void loop() {
  int backoff = 250;
  int attempt = 3;
  int statusCode = 503;

  sw.info("Backoff", backoff, "ms");
  sw.warn("HTTP", statusCode, "retry in", backoff, "ms", "attempt", attempt);

  delay(1000);
}

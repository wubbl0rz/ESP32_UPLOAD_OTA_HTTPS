#pragma once
#include "Arduino.h"
#include <HTTPClient.h>
#include <mutex>

class Spinner
{
private:
  std::string spinner = "|/-\\";
  int pos = 0;

public:
  char spin()
  {
    if (++pos >= spinner.length())
    {
      pos = 0;
    }

    return spinner[pos];
  }
};

String get_time()
{
  auto t = time(NULL);
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%D %H:%M:%S", localtime(&t));
  return String(timeStringBuff);
}

#ifdef __cplusplus
extern "C"
{
#endif
  uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

float read_temp()
{
  return (temprature_sens_read() - 32) / 1.8;
}

void http_get(String url)
{
  HTTPClient http;
  http.begin(url);
  http.GET();
  http.end();
}

int redirect_logs(const char *fmt, va_list args)
{
  char buffer[256];

  auto len = vsprintf(buffer, fmt, args);

  Serial.print(buffer);
  // LogHandler::SendAll(buffer);

  return len;
}
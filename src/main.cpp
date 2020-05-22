#include <Arduino.h>
#include <WiFi.h>
#include <helper.hpp>
#include <time.h>
#include <admin.hpp>
#include <secrets.hpp>

IPAddress ip(192, 168, 1, 210);
IPAddress gw(192, 168, 1, 1);
IPAddress mask(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

const char *HOSTNAME = "esp32_klingelsensor";

const char *NTP_SERVER = "192.168.1.1";
const char *TZ_INFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";

Spinner spinner;

#pragma region startup

void init_wifi()
{
  WiFi.config(ip, gw, mask, dns);
  WiFi.setHostname(HOSTNAME);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);

  WiFi.begin(SSID, PASSWORD);

  auto start = millis();

  while (!WiFi.isConnected())
  {
    if (millis() - start > 5000)
    {
      ESP.restart();
    }

    Serial.printf("\r %c", spinner.spin());
    delay(100);
  }

  Serial.println();

  configTime(0, 0, NTP_SERVER);

  Serial.printf("set ntp server: %s \n", NTP_SERVER);

  setenv("TZ", TZ_INFO, 1);

  Serial.print("ip: ");

  Serial.println(WiFi.localIP());
}

void launch_service_task()
{
  xTaskCreatePinnedToCore(
      [](void *optionalArgs) {
        unsigned long last = millis();
        while (true)
        {
          if (!WiFi.isConnected())
          {
            Serial.println("Reconnect to WiFi...");
            init_wifi();
          }

          if (millis() - last > 10000)
          {
            last = millis();
            Serial.println(get_time() + " : " + String(millis()) + " I AM ALIVE");
          }

          AdminServer::loop_admin_server();
        }
      },
      "ServiceTask", 10000, NULL, 1, NULL, 1);
}

void startup()
{
  Serial.println("BOOT...");

  init_wifi();
  AdminServer::launch_admin_server(8443);
  launch_service_task();

  Serial.println("wating for updates...");

  delay(5000);
  while (AdminServer::updateInProgress)
  {
    delay(100);
  }
}

#pragma endregion

auto doorbell = false;

void setup()
{
  Serial.begin(115200);
  startup();
  Serial.println(WiFi.getHostname());
  Serial.println("enter main loop");

  pinMode(GPIO_NUM_33, INPUT);        // telefon
  pinMode(GPIO_NUM_23, INPUT_PULLUP); // klingel

  //attachInterrupt(digitalPinToInterrupt(GPIO_NUM_33), null, CHANGE);
  attachInterrupt(
      digitalPinToInterrupt(GPIO_NUM_23), []() {
        doorbell = true;
      },
      FALLING);
}

void loop()
{
  // Serial.println(analogRead(GPIO_NUM_33));  // telefon
  // Serial.println(digitalRead(GPIO_NUM_23)); // klingel
  // Serial.println("=");
  // delay(1000);
}
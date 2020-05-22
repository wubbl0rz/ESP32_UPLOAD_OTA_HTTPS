#pragma once

#include <helper.hpp>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <WebsocketHandler.hpp>
#include <Update.h>

using namespace httpsserver;

namespace AdminServer
{

SSLCert *cert = NULL;
HTTPSServer *secureServer = NULL;
bool updateInProgress = false;

void handle_restart_get(HTTPRequest *req, HTTPResponse *res)
{
  res->println("restarting");
  secureServer->stop();
  delay(100);
  ESP.restart();
}

void handle_stats_get(HTTPRequest *req, HTTPResponse *res)
{
  auto temp = read_temp();
  res->println(String("{") +
               "\"temp\":\"" + String(temp) + "\"," +
               "\"time\":\"" + get_time() + "\"," +
               "\"uptime_ms\":\"" + millis() + "\"," +
               "\"hostname\":\"" + String(WiFi.getHostname()) + "\"" +
               String("}"));
}

void handle_update_post(HTTPRequest *req, HTTPResponse *res)
{
  Update.end();
  Update.clearError();

  std::string checksum;
  req->getParams()->getQueryParameter("checksum", checksum);

  Serial.printf("Checksum: %s  \n", checksum.c_str());
  res->printf("Checksum: %s  \n", checksum.c_str());

  if (!checksum.empty())
  {
    Update.setMD5(checksum.c_str());
  }

  if (!Update.begin(req->getContentLength(), U_FLASH))
  {
    ESP.restart();
  }

  updateInProgress = true;

  byte buffer[1024];
  auto len = req->getContentLength();
  auto fileLength = 0;
  auto updateProgress = 0;

  while (!req->requestComplete())
  {
    if (Update.hasError())
    {
      Update.printError(Serial);
      ESP.restart();
    }

    auto s = req->readBytes(buffer, 1024);
    fileLength += s;
    if (s > 0)
    {
      auto progress = ((fileLength * 100) / len);
      if (progress > updateProgress)
      {
        updateProgress = progress;
        Serial.printf("\r Update: %d / 100", progress);
        res->printf("Update: %d / 100 \n", progress);
      }

      if (Update.write(buffer, s) != s)
      {
        Update.printError(Serial);
        ESP.restart();
      }
    }
    else if (fileLength > 0)
    {
      ESP.restart();
    }
  }

  res->setStatusCode(200);

  if (!Update.end(true))
  {
    Update.printError(Serial);
    ESP.restart();
  }

  Serial.println("UPDATE FINISHED");
  Serial.println(Update.md5String());

  secureServer->stop();

  delay(100);

  ESP.restart();
}

void loop_admin_server()
{
  secureServer->loop();
}

void launch_admin_server(int port = 443)
{
  cert = new SSLCert();

  createSelfSignedCert(
      *cert,
      KEYSIZE_1024,
      "CN=KEKW,O=LUL,C=DE",
      "20190101000000",
      "20300101000000");

  secureServer = new HTTPSServer(cert, port, 20);

  ResourceNode *updateNode = new ResourceNode("/update", "POST", handle_update_post);
  ResourceNode *restartNode = new ResourceNode("/restart", "GET", handle_restart_get);
  ResourceNode *statsNode = new ResourceNode("/stats", "GET", handle_stats_get);
  secureServer->registerNode(updateNode);
  secureServer->registerNode(restartNode);
  secureServer->registerNode(statsNode);
  secureServer->start();
}
} // namespace AdminServer
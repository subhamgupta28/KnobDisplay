#ifndef AUTOMATA_ADD_ON_H
#define AUTOMATA_ADD_ON_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <vector>
#include <ESPmDNS.h>
#include "StompClient.h"
#include <WebSocketsClient.h>
#if ENABLE_SD_FILE_SERVER
#include "SDWebServer.h"
#endif

class AutomataAddOn;

struct MasterData
{
    String key0;
    String name;
    String id;
};

typedef std::vector<MasterData> MasterDataList;

class AutomataAddOn
{
public:
    static AutomataAddOn *instance;
    const char *HOST;
    int PORT;
    bool useHttps;
    AutomataAddOn(const char *HOST, int PORT);
    void begin();
    void loop();
    String getAutomations();
    void getAutomationsList();
    String getAutomationId(const String &name);
    MasterDataList getMasterDataList();
    bool getMasterDeviceByName(const char *searchName, String &outId, String &outKey);
    void setUseHttps(bool useHttps) { this->useHttps = useHttps; }
#if ENABLE_SD_FILE_SERVER
    void beginSDFileServer(AsyncWebServer *existingServer = nullptr);
#endif

private:
    
    void getMasterList();
    void parseJsonList(String jsonData);
    void splitAutomations(const String &input, String &names, String &ids);
    String getIdByName(const String &input, const String &searchName);
    bool sendHttp(const String &output, const String &endpoint, String &result);
    bool sendHttps(const String &output, const String &endpoint, String &result);
    // const char *HOST;
    // int PORT;
    AsyncWebServer server;
    MasterDataList masterDataList;
    String automations;
    HTTPClient http;
    String automationIds;
    String automationKeyIds;
#if ENABLE_SD_FILE_SERVER
    SDWebServer *sdweb; // pointer so it can be optional
#endif
};

#endif
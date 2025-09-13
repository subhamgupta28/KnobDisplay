#ifndef AUTOMATA_ADD_ON_H
#define AUTOMATA_ADD_ON_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <vector>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
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
    AutomataAddOn(const char *HOST, int PORT);
    void begin();
    void loop();
    String getAutomations();
    String getAutomationId(const String &name);
    MasterDataList getMasterDataList();
    bool getMasterDeviceByName(const char *searchName, String &outId, String &outKey);
#if ENABLE_SD_FILE_SERVER
    void beginSDFileServer(AsyncWebServer *existingServer = nullptr);
#endif

private:
    void getAutomationsList();
    void getMasterList();
    void parseJsonList(String jsonData);
    void splitAutomations(const String &input, String &names, String &ids);
    String getIdByName(const String &input, const String &searchName);
    bool sendHttp(const String &output, const String &endpoint, String &result);
    const char *HOST;
    int PORT;
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
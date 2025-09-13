#include "AutomataAddOn.h"

AutomataAddOn *AutomataAddOn::instance = nullptr;

AutomataAddOn::AutomataAddOn(const char *HOST, int PORT):HOST(HOST), PORT(PORT), server(80)
{
    instance = this;
}

void AutomataAddOn::begin()
{
#if ENABLE_SD_FILE_SERVER
    beginSDFileServer(&server);
#endif
}


#if ENABLE_SD_FILE_SERVER
void AutomataAddOn::beginSDFileServer(AsyncWebServer *existingServer)
{
    if (existingServer)
    {
        sdweb = new SDWebServer(*existingServer); // share
    }
    else
    {
        sdweb = new SDWebServer(); // create internal
    }
    sdweb->begin();
}
#endif

bool AutomataAddOn::getMasterDeviceByName(const char *searchName, String &outId, String &outKey)
{
    for (auto item : masterDataList)
    {
        if (item.name.equals(searchName))
        {
            outId = item.id;
            outKey = item.key0;
            return true; // found
        }
    }
    return false; // not found
}
void AutomataAddOn::parseJsonList(String jsonData)
{
    StaticJsonDocument<1024> doc;
    masterDataList.clear();

    // Parse JSON
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    JsonArray arr = doc.as<JsonArray>();

    for (JsonObject obj : arr)
    {
        MasterData md;
        md.key0 = (const char *)obj["key"];
        md.name = (const char *)obj["name"];
        md.id = (const char *)obj["id"];
        masterDataList.push_back(md);
    }
}
MasterDataList AutomataAddOn::getMasterDataList()
{
    getMasterList();
    return masterDataList;
}
void AutomataAddOn::getMasterList()
{
    JsonDocument req;
    req["list"] = "get";
    String jsonString;
    serializeJson(req, jsonString);
    String res;

    if (sendHttp(jsonString, "masterList", res))
    {
        Serial.println(res);
        parseJsonList(res);
    }
}
void AutomataAddOn::getAutomationsList()
{
    JsonDocument req;
    req["list"] = "get";
    String jsonString;
    serializeJson(req, jsonString);
    String res;

    if (sendHttp(jsonString, "automations", res))
    {
        Serial.println(res);
        String names, ids;
        automationKeyIds = res;
        splitAutomations(res, names, ids);
        automations = names;
        automationIds = ids;
    }
}
String AutomataAddOn::getIdByName(const String &input, const String &searchName)
{
    int start = 0;

    while (start < input.length())
    {
        // find the next comma
        int commaIndex = input.indexOf(',', start);
        if (commaIndex == -1)
            commaIndex = input.length();

        // extract "name:id"
        String pair = input.substring(start, commaIndex);

        int colonIndex = pair.indexOf(':');
        if (colonIndex > 0)
        {
            String name = pair.substring(0, colonIndex);
            String id = pair.substring(colonIndex + 1);

            // match name (case-sensitive)
            if (name == searchName)
            {
                return id;
            }
        }

        start = commaIndex + 1;
    }

    // not found
    return "";
}

void AutomataAddOn::splitAutomations(const String &input, String &names, String &ids)
{
    names = "";
    ids = "";
    int start = 0;

    while (start < input.length())
    {
        // find the next comma
        int commaIndex = input.indexOf(',', start);
        if (commaIndex == -1)
            commaIndex = input.length();

        // extract "name:id"
        String pair = input.substring(start, commaIndex);

        int colonIndex = pair.indexOf(':');
        if (colonIndex > 0)
        {
            String name = pair.substring(0, colonIndex);
            String id = pair.substring(colonIndex + 1);

            if (names.length() > 0)
            {
                names += ",";
                ids += ",";
            }
            names += name;
            ids += id;
        }

        start = commaIndex + 1;
    }
}

String AutomataAddOn::getAutomations()
{
    return automations;
}
String AutomataAddOn::getAutomationId(const String &name)
{
    return getIdByName(automationKeyIds, name);
}
bool AutomataAddOn::sendHttp(const String &output, const String &endpoint, String &result)
{

    HTTPClient http;
    result = "";

    http.begin("http://" + String(HOST) + ":" + String(PORT) + "/api/v1/main/" + endpoint);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000); // 5 seconds per request

    int httpCode = http.POST(output);

    if (httpCode > 0)
    {
        Serial.printf("[HTTP] POST code: %d\n", httpCode);

        result = http.getString(); // always capture response

        if (httpCode >= 200 && httpCode < 300)
        {
            http.end();
            return true; // success, exit early
        }
        else
        {
            Serial.printf("[HTTP] POST unexpected code: %d, body: %s\n",
                          httpCode, result.c_str());
        }
    }
    else
    {
        Serial.printf("[HTTP] POST attempt failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

    return false;
}
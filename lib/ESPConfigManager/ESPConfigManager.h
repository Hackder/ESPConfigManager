#pragma once

#include <ESP8266WebServer.h>
#include <string>
#include <map>
#include <EEPROM.h>
#include "index.h"

struct ConfigType
{
  const String type;
  int size;

  String (*serialize)(void *value);
  void (*deserialize)(String value, void *target);
};

const struct ConfigType intOption = {"int", sizeof(int), [](void *value)
                                     { return String(*(int *)value); },
                                     [](String value, void *target)
                                     {
                                       *(int *)target = value.toInt();
                                     }};

const struct ConfigType boolOption = {"bool", sizeof(bool), [](void *value)
                                      { return (*(bool *)value) ? String("true") : String("false"); },
                                      [](String value, void *target)
                                      {
                                        if (value == "true")
                                          *(bool *)target = true;
                                        else
                                          *(bool *)target = false;
                                      }};

const struct ConfigType stringOption = {"string", sizeof(int), [](void *value)
                                        { return String(*(String *)value); },
                                        [](String value, void *target)
                                        {
                                          *(String *)target = String(value);
                                        }};

#ifndef CONFIG
#define CONFIG \
  S(name)      \
  X(int, test)
#endif

const std::map<std::string, const ConfigType *> options = {
#define X(type, name) {#name, &type##Option},
#define S(name) {#name, &stringOption},
    CONFIG
#undef X
#undef S
};

const char *jsonOptions =
    "["
#define X(type, name) "{\"name\": \"" #name "\", \"type\": \"" #type "\"},"
#define S(name) "{\"name\": \"" #name "\", \"type\": \"string\"},"
    CONFIG
#undef X
#undef S
    "{}"
    "]";

struct Config
{
#define X(type, name) type name;
#define S(name) String name;
  CONFIG
#undef S
#undef X
};

Config readConfig()
{
  Config config;
  int offset = sizeof(size_t);

#define X(type, name)                \
  {                                  \
    EEPROM.get(offset, config.name); \
    offset += sizeof(type);          \
  }

#define S(name)                                     \
  {                                                 \
    int len = 0;                                    \
    EEPROM.get(offset, len);                        \
    offset += sizeof(int);                          \
    for (int i = 0; i < len; i++)                   \
    {                                               \
      config.name += (char)EEPROM.read(offset + i); \
    }                                               \
    offset += len;                                  \
  }

  CONFIG

#undef X
#undef S

  return config;
}

void writeConfig(const Config &config)
{
  int offset = sizeof(size_t);

#define X(type, name)                \
  {                                  \
    EEPROM.put(offset, config.name); \
    offset += sizeof(type);          \
  }

#define S(name)                                 \
  {                                             \
    int len = config.name.length();             \
    EEPROM.put(offset, len);                    \
    offset += sizeof(int);                      \
    for (int i = 0; i < len; i++)               \
    {                                           \
      EEPROM.write(offset + i, config.name[i]); \
    }                                           \
    offset += len;                              \
  }
  CONFIG
#undef X
#undef S

  EEPROM.commit();
}

String serializeConfig(Config &config)
{
  String result = "{";

#define X(type, name) \
  result += "\"" #name "\": " + type##Option.serialize(&config.name) + ", ";
#define S(name) \
  result += "\"" #name "\": \"" + config.name + "\", ";

  CONFIG

#undef X
#undef S

  result.remove(result.length() - 2);
  result += "}";
  return result;
}

Config parseConfig(String &config)
{
  Config result;
  std::map<std::string, void *> values;

#define X(type, name) values[#name] = &result.name;
#define S(name) values[#name] = &result.name;
  CONFIG
#undef X
#undef S

  unsigned int index = 0;
  while (config.indexOf("\"", index) != -1)
  {
    int nameStart = config.indexOf("\"", index) + 1;
    int nameEnd = config.indexOf("\"", nameStart);
    std::string name = std::string(config.substring(nameStart, nameEnd).c_str());

    auto configType = options.find(name)->second;

    int valueStart, valueEnd;
    String value;

    if (configType->type == "string")
    {

      valueStart = config.indexOf("\"", nameEnd + 1) + 1;
      valueEnd = config.indexOf("\"", valueStart);
      value = config.substring(valueStart, valueEnd);
    }
    else
    {
      valueStart = config.indexOf(":", nameEnd + 1) + 1;
      valueEnd = config.indexOf(",", valueStart);
      valueEnd = valueEnd == -1 ? config.indexOf("}", valueStart) : valueEnd;
      value = config.substring(valueStart, valueEnd);
      value.trim();
    }

    if (values.find(name) != values.end())
    {
      configType->deserialize(value, values[name]);
    }

    index = valueEnd + 1;
  }

  return result;
}

class ESPConfigManager
{
private:
  ESP8266WebServer server;

  const char *m_username;
  const char *m_password;

  Config currentConfig;

public:
  ESPConfigManager(const Config &defaultConfig, const char *username, const char *password, const size_t eepromSize = 1024)
  {
    m_username = username;
    m_password = password;
    EEPROM.begin(eepromSize);

    size_t shemaHash = std::hash<std::string>{}(jsonOptions);

    size_t currentHash;
    EEPROM.get(0, currentHash);

    if (currentHash != shemaHash)
    {
      EEPROM.put(0, shemaHash);
      writeConfig(defaultConfig);
      currentConfig = defaultConfig;
    }

    currentConfig = readConfig();

    server.on("/", HTTP_GET, [this]()
              { 
                if (!server.authenticate(m_username, m_password)) {
                  return server.requestAuthentication();
                }
                server.send(200, "text/html", IndexPage); });

    server.on("/configSchema", HTTP_GET, [this]()
              {
                // if (!server.authenticate(m_username, m_password)) {
                //   return server.requestAuthentication();
                // }
                server.send(200, "application/json", jsonOptions); });

    server.on("/config", HTTP_GET, [this]()
              {
    // if (!server.authenticate(m_username, m_password)) {
    //   return server.requestAuthentication();
    // }
              auto config = readConfig();
              server.send(200, "application/json", serializeConfig(config)); });

    server.on("/config", HTTP_POST, [this]()
              {

    // if (!server.authenticate(m_username, m_password)) {
    //   return server.requestAuthentication();
    // }
              auto requestBody = server.arg("plain");
              auto config = parseConfig(requestBody);
              writeConfig(config);
              currentConfig = config;
              server.send(200, "application/json", serializeConfig(config)); });
  }

  void start()
  {
    server.begin();
  }

  void handle()
  {
    server.handleClient();
  }

  const Config &getConfig()
  {
    return currentConfig;
  }
};

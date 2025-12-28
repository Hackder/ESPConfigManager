#include "index.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define string String

#define FOR_EACH_END(action, ...) __VA_ARGS__;
#define FOR_EACH_1(action, a, ...) action(a) FOR_EACH_END(action, __VA_ARGS__)
#define FOR_EACH_2(action, a, ...) action(a) FOR_EACH_1(action, __VA_ARGS__)
#define FOR_EACH_3(action, a, ...) action(a) FOR_EACH_2(action, __VA_ARGS__)
#define FOR_EACH_4(action, a, ...) action(a) FOR_EACH_3(action, __VA_ARGS__)
#define FOR_EACH_5(action, a, ...) action(a) FOR_EACH_4(action, __VA_ARGS__)
#define FOR_EACH_6(action, a, ...) action(a) FOR_EACH_5(action, __VA_ARGS__)
#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME
#define FOR_EACH(action, ...)                                                  \
    GET_MACRO(__VA_ARGS__, FOR_EACH_6, FOR_EACH_5, FOR_EACH_4, FOR_EACH_3,     \
              FOR_EACH_2, FOR_EACH_1, FOR_EACH_END)(action, __VA_ARGS__)

using f64 = double;

void config_manager_log(const String& message) {
    Serial.println("[ConfigManager]: " + message);
}

enum class ConfigFieldType {
    intType,
    floatType,
    stringType,
    boolType,
    buttonType,
};

String config_field_type_to_string(ConfigFieldType type) {
    switch (type) {
    case ConfigFieldType::intType:
        return "int";
    case ConfigFieldType::floatType:
        return "float";
    case ConfigFieldType::stringType:
        return "string";
    case ConfigFieldType::boolType:
        return "bool";
    case ConfigFieldType::buttonType:
        return "button";
    }
    return "unknown";
}

uint64_t config_field_type_id(ConfigFieldType type) {
    switch (type) {
    case ConfigFieldType::intType:
        return 10;
    case ConfigFieldType::floatType:
        return 11;
    case ConfigFieldType::stringType:
        return 12;
    case ConfigFieldType::boolType:
        return 13;
    case ConfigFieldType::buttonType:
        return 14;
    }
    return 5;
}

#define FIELD(type, name, ...) type name;
#define BUTTON(name, ...)
struct Config {
    CONFIG
};
#undef FIELD
#undef BUTTON

struct ButtonEvents {
    void (*on_click)(Config* current_config, Config* new_config) = nullptr;
};

#define FIELD(...)
#define BUTTON(name, ...) ButtonEvents name;
struct Buttons {
    CONFIG
};
#undef FIELD
#undef BUTTON

#define FIELD(...)
#define BUTTON(button_name, ...)                                               \
    if (strcmp(name, #button_name) == 0) {                                     \
        return &buttons->button_name;                                          \
    }
ButtonEvents* config_get_button_events(Buttons* buttons, const char* name) {
    CONFIG
    return nullptr;
}
#undef FIELD
#undef BUTTON

struct ConfigFieldOptions {
    f64 min = -INFINITY;
    f64 max = INFINITY;
    f64 step = 1;
    bool hide_slider = false;
    const char* title = nullptr;
    const char* description = nullptr;
    bool dont_save = false;

    ConfigFieldOptions(void (*init)(ConfigFieldOptions* options)) {
        init(this);
    }
};

struct ConfigField {
    const char* name;
    const ConfigFieldType type;
    const ConfigFieldOptions options;
};

#define SET_OPTION(setter) options->setter;
#define FIELD(field_type, field_name, ...)                                     \
    {.name = #field_name,                                                      \
     .type = ConfigFieldType::field_type##Type,                                \
     .options = ConfigFieldOptions([](ConfigFieldOptions* options) {           \
         FOR_EACH(SET_OPTION, __VA_ARGS__)                                     \
     })},
#define BUTTON(button_name, ...)                                               \
    {.name = #button_name,                                                     \
     .type = ConfigFieldType::buttonType,                                      \
     .options = ConfigFieldOptions([](ConfigFieldOptions* options) {           \
         FOR_EACH(SET_OPTION, __VA_ARGS__)                                     \
     })},

const ConfigField configFields[] = {CONFIG};
#undef FIELD
#undef BUTTON

#define FIELD(field_type, field_name, ...)                                     \
    if (strcmp(name, #field_name) == 0) {                                      \
        return (void*)&config->field_name;                                     \
    }
#define BUTTON(name, ...)

void* config_get_ptr(Config* config, const char* name) {
    CONFIG

    return nullptr;
}
#undef FIELD
#undef BUTTON

uint64_t simple_hash(uint64_t key) {
    // FNV-1a inspired constants
    const uint64_t prime = 0x100000001b3;
    const uint64_t offset = 0xcbf29ce484222325;

    // Mix the bits using XOR, multiplication, shifts and rotations
    uint64_t hash = offset;

    // Split the key and mix in chunks to improve distribution
    hash ^= (key & 0xFFFFFFFF);
    hash *= prime;
    hash ^= (key >> 32);
    hash *= prime;

    // Additional avalanche step to improve bit distribution
    hash ^= hash >> 23;
    hash *= 0x2127599bf4325c37ULL;
    hash ^= hash >> 47;

    return hash;
}

uint64_t config_hash(const Config* config) {
    uint64_t hash = 0;
    for (auto& field : configFields) {
        if (field.options.dont_save) {
            continue;
        }

        if (field.type == ConfigFieldType::buttonType) {
            continue;
        }

        hash =
            simple_hash(hash + simple_hash(config_field_type_id(field.type)));
    }
    return hash;
}

// Function to convert ConfigFieldOptions to JSON using JsonDocument
static String config_options_to_json(ConfigFieldOptions* options) {
    JsonDocument doc;
    if (options->min != -INFINITY) {
        doc["min"] = options->min;
    }
    if (options->max != INFINITY) {
        doc["max"] = options->max;
    }
    doc["step"] = options->step;
    if (options->hide_slider) {
        doc["hide_slider"] = options->hide_slider;
    }
    if (options->description != nullptr) {
        doc["description"] = options->description;
    }
    if (options->title != nullptr) {
        doc["title"] = options->title;
    }
    if (options->dont_save) {
        doc["dont_save"] = options->dont_save;
    }
    String json;
    serializeJson(doc, json);
    return json;
}

// Function to convert Config to JSON using JsonDocument
static String config_to_json(Config* config) {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    int config_fields = sizeof(configFields) / sizeof(ConfigField);
    for (int i = 0; i < config_fields; i++) {
        JsonObject field = array.add<JsonObject>();
        field["name"] = configFields[i].name;
        field["type"] = config_field_type_to_string(configFields[i].type);

        void* field_ptr = config_get_ptr(config, configFields[i].name);
        if (field_ptr) {
            switch (configFields[i].type) {
            case ConfigFieldType::intType: {
                field["value"] = *static_cast<int*>(field_ptr);
                break;
            }
            case ConfigFieldType::floatType: {
                field["value"] = *static_cast<float*>(field_ptr);
                break;
            }
            case ConfigFieldType::stringType: {
                field["value"] = *static_cast<String*>(field_ptr);
                break;
            }
            case ConfigFieldType::boolType: {
                field["value"] = *static_cast<bool*>(field_ptr);
                break;
            }
            case ConfigFieldType::buttonType: {
                // Buttons don't store values
                break;
            }
            }
        }

        field["options"] = serialized(config_options_to_json(
            (ConfigFieldOptions*)&configFields[i].options));
    }

    String json;
    serializeJson(doc, json);
    return json;
}

static void config_load_from_json(Config* config, const JsonObject& json_obj) {
    for (JsonPair kv : json_obj) {
        const char* key = kv.key().c_str();

        // Find matching config field
        for (auto& field : configFields) {
            if (strcmp(field.name, key) != 0)
                continue;

            void* field_ptr = config_get_ptr(config, key);
            if (!field_ptr)
                break;

            switch (field.type) {
            case ConfigFieldType::intType: {
                if (kv.value().is<int>()) {
                    int val = kv.value().as<int>();
                    if (field.options.min != 0 || field.options.max != 0) {
                        val = constrain(val, field.options.min,
                                        field.options.max);
                    }
                    *static_cast<int*>(field_ptr) = val;
                } else {
                    config_manager_log(
                        ("Invalid value for field " + String(key)).c_str());
                }
                break;
            }
            case ConfigFieldType::floatType: {
                if (kv.value().is<double>()) {
                    double val = kv.value().as<double>();
                    if (field.options.min != 0 || field.options.max != 0) {
                        val = constrain(val, field.options.min,
                                        field.options.max);
                    }
                    *static_cast<float*>(field_ptr) = (float)val;
                } else {
                    config_manager_log(
                        ("Invalid value for field " + String(key)).c_str());
                }
                break;
            }
            case ConfigFieldType::stringType: {
                if (kv.value().is<const char*>()) {
                    *static_cast<String*>(field_ptr) =
                        kv.value().as<const char*>();
                } else {
                    config_manager_log(
                        ("Invalid value for field " + String(key)).c_str());
                }
                break;
            }
            case ConfigFieldType::boolType: {
                if (kv.value().is<bool>()) {
                    *static_cast<bool*>(field_ptr) = kv.value().as<bool>();
                } else {
                    config_manager_log(
                        ("Invalid value for field " + String(key)).c_str());
                }
                break;
            }
            case ConfigFieldType::buttonType: {
                // Buttons don't store values
                break;
            }
            }
            break;
        }
    }
}

static bool config_write_to_eeprom(Config* config) {
    uint64_t hash = config_hash(config);
    EEPROM.put(0, hash);
    int offset = sizeof(hash);

    config_manager_log("Writing configuration to EEPROM");

    for (auto& field : configFields) {
        void* field_ptr = config_get_ptr(config, field.name);
        if (!field_ptr) {
            continue;
        }

        if (field.options.dont_save) {
            continue;
        }

        switch (field.type) {
        case ConfigFieldType::intType: {
            EEPROM.put(offset, *static_cast<int*>(field_ptr));
            offset += sizeof(int);
            break;
        }
        case ConfigFieldType::floatType: {
            EEPROM.put(offset, *static_cast<float*>(field_ptr));
            offset += sizeof(float);
            break;
        }
        case ConfigFieldType::stringType: {
            String str = *static_cast<String*>(field_ptr);
            int len = str.length();
            EEPROM.put(offset, len);
            offset += sizeof(int);
            for (int i = 0; i < len; i++) {
                EEPROM.put(offset + i, str[i]);
            }
            offset += len;
            break;
        }
        case ConfigFieldType::boolType: {
            EEPROM.put(offset, *static_cast<bool*>(field_ptr));
            offset += sizeof(bool);
            break;
        }
        case ConfigFieldType::buttonType: {
            // Buttons don't store values
            break;
        }
        }
    }

    EEPROM.commit();

    return true;
}

static bool config_load_from_eeprom(Config* config) {
    config_manager_log("Loading configuration from EEPROM");

    uint64_t hash;
    EEPROM.get(0, hash);
    if (hash != config_hash(config)) {
        config_manager_log("Configuration hash mismatch");
        return false;
    }
    int offset = sizeof(hash);

    for (auto& field : configFields) {
        void* field_ptr = config_get_ptr(config, field.name);
        if (!field_ptr) {
            continue;
        }

        if (field.options.dont_save) {
            continue;
        }

        switch (field.type) {
        case ConfigFieldType::intType: {
            EEPROM.get(offset, *static_cast<int*>(field_ptr));
            offset += sizeof(int);
            break;
        }

        case ConfigFieldType::floatType: {
            EEPROM.get(offset, *static_cast<float*>(field_ptr));
            offset += sizeof(float);
            break;
        }

        case ConfigFieldType::stringType: {
            int len;
            EEPROM.get(offset, len);
            offset += sizeof(int);
            String str;
            for (int i = 0; i < len; i++) {
                char c;
                EEPROM.get(offset + i, c);
                str += c;
            }
            *static_cast<String*>(field_ptr) = str;
            offset += len;
            break;
        }
        case ConfigFieldType::boolType: {
            EEPROM.get(offset, *static_cast<bool*>(field_ptr));
            offset += sizeof(bool);
            break;
        }
        case ConfigFieldType::buttonType: {
            // Buttons don't store values
            break;
        }
        }
    }

    return true;
}

void config_save_builtin(Config* current_config, Config* new_config) {
    *current_config = *new_config;
    bool result = config_write_to_eeprom(current_config);
    if (!result) {
        config_manager_log("Failed to save configuration to EEPROM");
    }
}

class ESPConfigManager {
    AsyncWebServer server = {80};
    AsyncAuthenticationMiddleware auth;

    bool authenticate = false;

  public:
    ESPConfigManager() {};
    Config config = {};
    Buttons buttons = {};

    bool init(Config defaultConfig, const char* username, const char* password,
              const size_t eeprom_size = 1024) {

        this->authenticate = strlen(username) > 0 && strlen(password) > 0;

        if (this->authenticate) {
            this->auth.setUsername(username);
            this->auth.setPassword(password);
            this->auth.setRealm("ESPConfigManager");
            this->auth.setAuthFailureMessage("Bad username or password");
            this->auth.setAuthType(AsyncAuthType::AUTH_BASIC);
            this->auth.generateHash();
        }

        bool result = EEPROM.begin(eeprom_size);
        if (!result) {
            config_manager_log("Failed to initialize EEPROM");
            return false;
        }

        uint64_t stored_hash;
        EEPROM.get(0, stored_hash);
        uint64_t hash = config_hash((Config*)nullptr);
        config_manager_log("Stored hash: " + String(stored_hash));
        config_manager_log("Calculated hash: " + String(hash));
        if (stored_hash != hash) {
            config_manager_log("Configuration hash mismatch, setting defaults");
            bool result = config_write_to_eeprom(&defaultConfig);
            if (!result) {
                config_manager_log("Failed to write default configuration");
                return false;
            }
            this->config = defaultConfig;
        } else {
            bool result = config_load_from_eeprom(&this->config);
            if (!result) {
                config_manager_log("Failed to load configuration from EEPROM");
                return false;
            }
            config_manager_log("Configuration loaded from EEPROM");
        }

        return true;
    }

    void start() {
        int index_page_size = strlen(IndexPage);
        server
            .on("/", HTTP_GET,
                [index_page_size](AsyncWebServerRequest* request) {
                    request->send(200, "text/html", (uint8_t*)IndexPage,
                                  index_page_size);
                })
            .addMiddleware(&this->auth);

        // server
        //     .on("/config", HTTP_GET,
        //         [this](AsyncWebServerRequest* request) {
        //             request->send(200, "application/json",
        //                           config_to_json(&this->config));
        //         })
        //     .addMiddleware(&this->auth);

        // server
        //     .on("/event", HTTP_OPTIONS,
        //         [](AsyncWebServerRequest* request) {
        //             request->sendHeader("Access-Control-Allow-Methods",
        //             "POST");
        //             request->sendHeader("Access-Control-Allow-Headers",
        //                                 "Content-Type");
        //             request->send(200, "application/json", "{}");
        //         })
        //     .addMiddleware(&this->auth);

        // server
        //     .on("/event", HTTP_POST,
        //         [this](AsyncWebServerRequest* request) {
        //             // Accepts an object of the form: {"name"; "button_name",
        //             // "event": "click", "config": {...}}
        //
        //             JsonDocument doc;
        //             DeserializationError error =
        //                 deserializeJson(doc, request->arg("plain"));
        //             if (error) {
        //                 request->send(400, "application/json",
        //                               "{\"error\": \"Invalid JSON\"}");
        //                 return;
        //             }
        //
        //             if (!doc["name"].is<JsonString>() ||
        //                 !doc["event"].is<JsonString>() ||
        //                 !doc["config"].is<JsonObject>()) {
        //                 request->send(400, "application/json",
        //                               "{\"error\": \"Invalid required
        //                               keys\"}");
        //                 return;
        //             }
        //
        //             const String name = doc["name"];
        //             const String event = doc["event"];
        //
        //             if (event != "click") {
        //                 request->send(400, "application/json",
        //                               "{\"error\": \"Invalid event\"}");
        //                 return;
        //             }
        //
        //             ButtonEvents* events =
        //                 config_get_button_events(&this->buttons,
        //                 name.c_str());
        //
        //             if (events == nullptr) {
        //                 request->send(400, "application/json",
        //                               "{\"error\": \"Invalid button
        //                               name\"}");
        //                 return;
        //             }
        //
        //             if (events->on_click) {
        //                 Config new_config = {};
        //
        //                 config_load_from_json(&new_config, doc["config"]);
        //
        //                 events->on_click(&this->config, &new_config);
        //             } else {
        //                 config_manager_log("No event handler for button " +
        //                                    name);
        //             }
        //
        //             request->send(200, "application/json",
        //                           config_to_json(&this->config));
        //         })
        //     .addMiddleware(&this->auth);

        // Setup the server
        // server.on("/", HTTP_GET, [this]() {
        //     if (this->authenticate &&
        //         !server.authenticate(this->username, this->password)) {
        //         return server.requestAuthentication();
        //     }
        //     server.send(200, "text/html", IndexPage);
        // });
        //
        // server.on("/config", HTTP_GET, [this]() {
        //     if (this->authenticate &&
        //         !server.authenticate(this->username, this->password)) {
        //         return server.requestAuthentication();
        //     }
        //     server.send(200, "application/json",
        //     config_to_json(&this->config));
        // });
        //
        // server.on("/event", HTTP_OPTIONS, [this]() {
        //     if (this->authenticate &&
        //         !server.authenticate(this->username, this->password)) {
        //         return server.requestAuthentication();
        //     }
        //     server.sendHeader("Access-Control-Allow-Methods", "POST");
        //     server.sendHeader("Access-Control-Allow-Headers",
        //     "Content-Type"); server.send(200, "application/json", "{}");
        // });
        //
        // server.on("/event", HTTP_POST, [this]() {
        //     if (this->authenticate &&
        //         !server.authenticate(this->username, this->password)) {
        //         return server.requestAuthentication();
        //     }
        //
        //     // Accepts an object of the form: {"name"; "button_name",
        //     "event":
        //     // "click", "config": {...}}
        //
        //     JsonDocument doc;
        //     DeserializationError error =
        //         deserializeJson(doc, server.arg("plain"));
        //     if (error) {
        //         server.send(400, "application/json",
        //                     "{\"error\": \"Invalid JSON\"}");
        //         return;
        //     }
        //
        //     if (!doc["name"].is<JsonString>() ||
        //         !doc["event"].is<JsonString>() ||
        //         !doc["config"].is<JsonObject>()) {
        //         server.send(400, "application/json",
        //                     "{\"error\": \"Invalid required keys\"}");
        //         return;
        //     }
        //
        //     const String name = doc["name"];
        //     const String event = doc["event"];
        //
        //     if (event != "click") {
        //         server.send(400, "application/json",
        //                     "{\"error\": \"Invalid event\"}");
        //         return;
        //     }
        //
        //     ButtonEvents* events =
        //         config_get_button_events(&this->buttons, name.c_str());
        //
        //     if (events == nullptr) {
        //         server.send(400, "application/json",
        //                     "{\"error\": \"Invalid button name\"}");
        //         return;
        //     }
        //
        //     if (events->on_click) {
        //         Config new_config = {};
        //
        //         config_load_from_json(&new_config, doc["config"]);
        //
        //         events->on_click(&this->config, &new_config);
        //     } else {
        //         config_manager_log("No event handler for button " +
        //         name);
        //     }
        //
        //     server.send(200, "application/json",
        //     config_to_json(&this->config));
        // });
        //
        // server.on("/logstream", HTTP_GET, [this]() {
        //     if (this->authenticate &&
        //         !server.authenticate(this->username, this->password)) {
        //         return server.requestAuthentication();
        //     }
        // });
        //
        // server.begin();

        server.begin();
    }

    void save() {
        config_save_builtin(&this->config, &this->config);
    }
};

#undef string

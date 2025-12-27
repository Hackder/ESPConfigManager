#include "Arduino.h"
#include <WiFi.h>

// Toto je nova specifikacia konfiguracie. Funguje takto:
// Bud sa definuje FIELD. Toto je nejaky vstup. Bud textove pole alebo cislo,
// alebo prepinac (ak je to bool). Potom existuje aj BUTTON. Toto je tlacitko.
// Ukazeme si ako to funguje
//
// FIELD:
//   Ked sa definuje FIELD, musi sa specifikovat minimalne 2 veci. To je Typ
//   premennej a nazov premennej. FIELD(int, id, )
//
//   !! POZOR !! Vzdy to musi koncit ciarkou, a az potom je zatvorka. Teda nieco
//   taketo: FIELD(int, id) nie je spravne. Musi byt FIELD(int, id, )
//
//   Potom nasleduje niekolko moznych parametrov. Titeto niesu povinne a na
//   poradi nezalezi. Existuju tieto parametre:
//     - title - specifikuje nazov ktory za zobrazi nad textovym polom v online
//       editore. Pokial nieje specifikovany pouzije sa nazov premennej.
//     - description - dlhsi popis danej moznosti. Zobrazi sa hned pod textovym
//       polom mensim pismom.
//     - min - specifikuje minimalnu hodnotu.
//       Funguje pre policka typu float a int
//     - max - specifikuje maximalnu hodnotu. FUnguje pre policka typu float a
//       int Ak sa specifikuje aj min aj max, tak sa zobrazi v editore posuvnik,
//       ktorym sa da hodnota menit.
//     - step - urci po akom intervale sa meni hodnota posuvnikom.
//       Defaultne je to 1.
//       No napriklad ak by sme mali premennu float, ktora by mala byt od 0 po 1
//       tak by sa oplatilo nastavit step na 0.01 napriklad, aby sme vedeli
//       vyuzivat aj posuvnik
//     - dont_save - Ak sa tento parameter nastavi na true, tak dana polozka sa
//       nebude ukladat do EEPROM. Teda po restartovani nadobudne zase defaultnu
//       hodnotu. Toto je uzitocne ked je dana polozka myslena ako parameter pre
//       nejake ine tlacitko
//
//
// BUTTON:
//   V tejto verzi nerozlisujeme tlacika. Tlacitko na ulozenie konfiguracie a
//   akekolvek ine funguje presne rovnako. To znamena ze vpohode mozeme
//   ConfigManager pouzivat aj bez ukladania, len ako metodu volania funkcii z
//   online rozhrania.
//
//   Ked sa definuje BUTTON, musi sa specifikovat minimalne jeden parameter. To
//   je meno tlacitka. Toto meno musi byt spravne meno premennej (teda
//   nemoze obsahovat medzery napriklad) BUTTON(save_config, )
//
//   !! POZOR !! Vzdy to musi koncit ciarkou, a az potom je zatvorka. Teda nieco
//   taketo: BUTTON(save_config) nie je spravne. Musi byt BUTTON(save_config, )
//
//   Potom nasleduje niekolko moznych parametrov. Su to tie iste ako pri FIELD,
//   no nie vsetky funguju. v tomto pripade funguje iba jeden, a to `title`
//   ktory nastavy nazov tlacitka ktory sa zobrazi na stranke. Pokial nie je
//   nastaveny, pouzije sa meno tlacitka.
//
//
// Zvysok vysvetlenia pokracuje v komentaroch v kode.

// Toto je ukazkova definicia configu
#define CONFIG                                                                 \
    FIELD(int, id, )                                                           \
    FIELD(int, port, min = -9000.0, max = 90000, title = "The port",           \
          description = "This is the most awesome port", )                     \
    FIELD(float, some_float, min = 0.0, max = 100.0, step = 0.1,               \
          title = "Some float", )                                              \
    FIELD(float, other_float, min = 0.0, max = 100.0, step = 0.1,              \
          title = "Other float", hide_slider = true, )                         \
    FIELD(bool, some_bool, title = "Some bool",                                \
          description = "More information about this option", )                \
    FIELD(string, some_string, title = "Some string", )                        \
    BUTTON(save_config, title = "Save configuration", )                        \
    FIELD(string, data_to_log, title = "Data to log",                          \
          description = "Some data to log to the serial", dont_save = true, )  \
    BUTTON(log_data, title = "Log data", )
// Vsimnime si ze tento include musi nasledovat za definiciou configu
#include <ESPConfigManager.h>

ESPConfigManager configManager;

// Definujeme si zakladne hodnoty pre config. Tieto hodnoty sa nastavia ked:
// - spustime config manager prvy krat na danom zariadeni
// - zmenime typy, alebo pridame/odstranime hodnoty ktore sa ukladaju do EEPROM.
//   Teda pokial zmenime nejaku hodnotu ktora ma nastavene `dont_save` tak veci
//   ktore boli ulozene zostanu
const Config defaultConfig = {
    .id = 10,
};

// Toto je funkcia ktora sa zavola ked stlacim tlacitko. To ze sa ma tato
// zavolat je urcene v setupe. Funkcia dostane dva parametre. Prvy z nich
// je aktualny config, druhy obsahuje take hodnoty ake uzivatel prave vyplnil
// na stranke. Po skonceni tejto funkcie, configManager odosle stav configu
// naspat na webstranku a ten sa zobrazi pouzivatelovy. Teda keby tato funkcia
// trvala dlhsie, tak tlacitko by nacitavalo dlhsie.
void log_data_on_click(Config* current_config, Config* new_config) {
    // Vypiseme danu hodnotu do Serialu. (To je pointa tejto ukazkovej funkcie.
    // aby sme si ukazali ako sa daju volat ine veci tlacitkami ako len
    // ukladanie configu do EEPROM) Po dokonceni tejto funkcie, config manager
    // posle aktualny stav configu naspat do prehliadaca a tam sa nastavy ako
    // aktualny. Kedze sme nezmenili `current_config`, tak sa nam vsetok text v
    // `data_to_log` strati. Toto je ale to co v tomto pripade chceme. Chceme
    // aby sa po stlaceni tlacidla jeho parametre naspat vynulovali. Pokial by
    // sme to nechceli, tak musime nastavit aktualny config na ten novy. To by
    // vyzeralo takto: *current_config = *new_config;
    Serial.println(new_config->data_to_log);
}

const char* ssid = "ssid";
const char* password = "pass";

void setup() {
    // Najprv inicializujeme serial, aby sme mohli vydiet vsetky chybove hlasky
    // (ak by nejake boli) z config manageru
    Serial.begin(9600);

    // Potom inicializujeme config manager. Nechal som username ako prazdny
    // string, teda som akokeby vypol prihlasenie. Teda uz otvorenie config
    // manageru nebude vyzadovat prihlasenie. Keby ho inicializujem takto:
    // configManager.init(defaultConfig, "admin", "heslo");
    // Tak by otvorenie vyzadovalo prihlasenie menom "admin" a heslom "heslo"
    // V tomto momente sa aj nacita aktualna konfiguracia z EEPROM. Teda po
    // zavolani init, uz mozeme pouzivat hodnoty z configManageru. Mohlo by tam
    // byt teda aj prihlasenie na WiFi napriklad.
    configManager.init(defaultConfig, "", "");
    // Tieto dva riadky nastavia funkcie ktore sa maju zavolat po stlaceni
    // daneho tlacitka. Toto prve tlacitko sme si uz presli. Iba vypise aktualnu
    // hodnotu v `data_to_log` do serialu.
    configManager.buttons.log_data.on_click = log_data_on_click;
    // Tuto nastavime funkciu pre tlacitko na ulozenie configu. Tato funkcia je
    // zabudovana do config manageru, teda ju nemusime programovat. Vola sa
    // `config_save_builtin`. Keby chceme robit nieco specialne pri ukladani
    // parametrov, mozeme si definovat vlastnu. Tato definicia z kniznice vyzera
    // takto:
    // void config_save_builtin(Config* current_config, Config* new_config) {
    //     *current_config = *new_config;
    //     bool result = config_write_to_eeprom(current_config);
    //     if (!result) {
    //         config_manager_log("Failed to save configuration to EEPROM");
    //     }
    // }
    configManager.buttons.save_config.on_click = config_save_builtin;

    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Tymto riadkom zapneme webserver ktory bude cakat a odpovedat klientom
    configManager.start();

    // ---------------------------------------------------------------------
    // !! Nasleduje ukazka pouzivania. Toto netreba davat do setupu. Je to len
    // ukazka.

    // Takto vieme precitat nejaku hodnotu z configu
    Serial.println(configManager.config.id);

    // Ak nejaku hodnotu upravime, mozeme zavolat pomocnu funkciu `save`
    // Toto je relevantne len ak chceme upravovat nejaku hodnotu z programu
    configManager.config.id = 10;
    configManager.save();
}

void loop() {
    configManager.handle();
}

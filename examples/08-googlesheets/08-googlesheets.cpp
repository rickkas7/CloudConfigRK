#include "CloudConfigRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

const size_t EEPROM_OFFSET = 0;

void logJson();

void setup() {
    // This two lines are here so you can see the debug logs. You probably
    // don't want them in your code.
    waitFor(Serial.isConnected, 10000);
    delay(2000);

    // You must call this from setup!
    CloudConfig::instance()
        .withDataCallback([]() {
            Log.info("dataCallback");        
            logJson();
        })
        .withUpdateFrequencyAtRestart()
        //.withUpdateFrequency(24h)
        .withUpdateMethod(new CloudConfigUpdateWebhook("ConfigSpreadsheet"))
        .withStorageMethod(new CloudConfigStorageEEPROM<256>(EEPROM_OFFSET))
        .setup();
}

void loop() {
    // You must call this from loop!
    CloudConfig::instance().loop();
}


void logJson() {
    Log.info("configuration:");

    JSONObjectIterator iter(CloudConfig::instance().getJSONValue());
    while(iter.next()) {
        Log.info("  key=%s value=%s", 
          (const char *) iter.name(), 
          (const char *) iter.value().toString());
    }

}

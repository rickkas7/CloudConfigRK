#include "CloudConfigRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

// particle call boron5 setConfig '{"a":123,"b":"testing","c":true,"d",12.4,"e":[1,2,3],"f":{"f1":1,"f2":2}}'

void logJson() {
    if (CloudConfig::instance().getJSONValueForKey("a").isValid()) {
        Log.info("a=%d b=%s c=%s d=%lf",
            CloudConfig::instance().getInt("a"),
            CloudConfig::instance().getString("b"),
            CloudConfig::instance().getBool("c") ? "true" : "false",
            CloudConfig::instance().getDouble("d"));

        JSONValue array = CloudConfig::instance().getJSONValueForKey("e");
        JSONArrayIterator iter(array);
        for(size_t ii = 0; iter.next(); ii++) {
            Log.info("%u: %s", ii, iter.value().toString().data());
        }

        JSONValue obj = CloudConfig::instance().getJSONValueForKey("f");
        Log.info("f1=%d f2=%d",
            CloudConfigStorage::getJSONValueForKey(obj, "f1").toInt(),
            CloudConfigStorage::getJSONValueForKey(obj, "f2").toInt());
    }
    else {
        Log.info("no config set");
    }
}

void setup() {
    // This two lines are here so you can see the debug logs. You probably
    // don't want them in your code.
    waitFor(Serial.isConnected, 10000);
    delay(2000);

    // You must call this from setup!
    CloudConfig::instance()
        .withDataCallback(logJson)
        .withUpdateMethod(new CloudConfigUpdateFunction("setConfig"))
        .withStorageMethod(new CloudConfigStorageFile<256>("/usr/cloudconfig"))
        .setup();
}

void loop() {
    // You must call this from loop!
    CloudConfig::instance().loop();
}

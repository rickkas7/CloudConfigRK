#include "CloudConfigRK.h"

CloudConfig *CloudConfig::_instance;

//
// CloudConfigStorageData
// 

CloudConfigStorageData::CloudConfigStorageData(CloudConfigDataHeader *header, size_t dataSize) : header(header), dataSize(dataSize) {

}

CloudConfigStorageData &CloudConfigStorageData::withData(CloudConfigDataHeader *header, size_t dataSize) {
    this->header = header;
    this->dataSize = dataSize;
    return *this;
}


void CloudConfigStorageData::validate() {
    if (header->magic == CloudConfig::DATA_MAGIC &&
        header->headerSize == (uint8_t)sizeof(CloudConfigDataHeader) &&
        header->dataSize == (uint16_t)dataSize) {
        // Looks valid
    }
    else {
        // Not valid, reinitialize
        memset(header, 0, sizeof(CloudConfigDataHeader));
        header->magic = CloudConfig::DATA_MAGIC;
        header->headerSize = (uint8_t)sizeof(CloudConfigDataHeader);
        header->dataSize = (uint16_t)dataSize;

        memset(getJsonData(), 0, dataSize);
    }

    parse();
}

const char * const CloudConfigStorageData::getJsonData() const {
    return &((const char * const)header)[sizeof(CloudConfigDataHeader)];
}

char *CloudConfigStorageData::getJsonData() {
    return &((char *)header)[sizeof(CloudConfigDataHeader)];
}

bool CloudConfigStorageData::updateData(const char *json) {
    size_t jsonLen = strlen(json);
    if (jsonLen < (dataSize - 1)) {
        strcpy(getJsonData(), json);
        parse();
        return save();
    }
    else {
        // Too long, reject
        return false;
    }
}


//
// CloudConfigStorage
//
// [static]
JSONValue CloudConfigStorage::getJSONValueForKey(JSONValue parentObj, const char *key) {
    JSONObjectIterator iter(parentObj);
    while(iter.next()) {
        if (strcmp((const char *)iter.name(), key) == 0) {
            // Found
            return iter.value();
        }
    }
    return JSONValue(); // Invalid JSONValue
}

// [static]
JSONValue CloudConfigStorage::getJSONValueAtIndex(JSONValue parentObj, size_t index) {
    JSONObjectIterator iter(parentObj);
    size_t ii = 0;
    while(iter.next()) {
        if (ii == index) {
            // Found
            return iter.value();
        }
    }
    return JSONValue(); // Invalid JSONValue
}


//
// CloudConfig
//

CloudConfig &CloudConfig::instance() {
    if (!_instance) {
        _instance = new CloudConfig();
    }
    return *_instance;
}


CloudConfig::CloudConfig() {

}

CloudConfig::~CloudConfig() {

}

void CloudConfig::setup() {
    if (!storageMethod) {
        // A storage method is always required
        return;
    }

    storageMethod->setup();
    
    if (updateMethod) {
        // An update method is not required. CloudConfigStorageStatic for example
        // cannot be update because the data is in code flash.
        updateMethod->setup();

        // However the state machine only runs when both storageMethod and
        // updateMethod are non-null. Without an update handler the state
        // machine can't do anything
        stateHandler = &CloudConfig::stateStart;

    }

}

void CloudConfig::loop() {
    // Give loop time to the storage and update methods if necessary
    storageMethod->loop();
    if (updateMethod) {
        updateMethod->loop();
    }

    // State machine
    if (stateHandler) {
        stateHandler(*this);
    }
}

void CloudConfig::stateStart() {
    // Handle retrieve on start

    if (storageMethod->hasJsonData() && dataCallback) {
        // Send notification if enabled and we have data
        dataCallback();
    }

    // Handle retrieve periodically
    stateHandler = &CloudConfig::stateWaitCloudConnected;
}

void CloudConfig::stateWaitCloudConnected() {
    if (!Particle.connected() || !Time.isValid()) {
        return;
    }

    Log.info("cloud connected");

    // Cloud connection is up (and we have the time, which we will need shortly)
    stateHandler = &CloudConfig::stateWaitAfterCloudConnected;
    stateTime = millis();
}

void CloudConfig::stateWaitAfterCloudConnected() {
    if (millis() - stateTime < updateMethod->waitAfterCloudConnectedMs) {
        return;
    }

    // We're cloud connected and waited briefly (2 seconds) to make
    // sure registration is complete. This delay isn't necessary in 2.0.0
    // or later but it doesn't hurt.
    if (!storageMethod->hasJsonData() || updateFrequency == UPDATE_AT_RESTART) {
        // Need to update data as we do not have it, or we should update at every restart
        Log.info("no data or update at restart");
        stateHandler = &CloudConfig::stateStartUpdate;
    }
    else {
        Log.info("wait for update");
        stateTime = millis();
        stateHandler = &CloudConfig::stateWaitToUpdate;
    }
}

void CloudConfig::stateWaitToUpdate() {
    if (millis() - stateTime < 10000) {
        return;
    }

    // Run these checks every 10 seconds
    if (Time.isValid() && updateFrequency > 0) {
        if (Time.now() - storageMethod->getDataHeader()->lastCheck > updateFrequency) {
            // Time to update
            Log.info("checking for time update");
            stateHandler = &CloudConfig::stateStartUpdate;
        }
    }
}

void CloudConfig::stateStartUpdate() {
    Log.info("stateStartUpdate");
    storageMethod->getDataHeader()->lastCheck = Time.now();
    updateDataStatus = UpdateDataStatus::IN_PROGRESS;
    stateTime = millis();
    stateHandler = &CloudConfig::stateWaitUpdateComplete;

    updateMethod->startUpdate();
}

void CloudConfig::stateWaitUpdateComplete() {
    if (updateDataStatus == UpdateDataStatus::IN_PROGRESS) {
        if (millis() - stateTime > updateMethod->updateTimeoutMs) {
            // Timeout
            Log.info("stateWaitUpdateComplete timeout");
            updateDataStatus = UpdateDataStatus::TIMEOUT;
            stateTime = millis();
            stateHandler = &CloudConfig::stateWaitToUpdate;
            return;
        }
        else {
            // Still in progress
            return;
        }
    }

    // Not in progress anymore. 
    Log.info("stateWaitUpdateComplete complete");
    
    // Wait to update again
    stateTime = millis();
    stateHandler = &CloudConfig::stateWaitToUpdate;
}


bool CloudConfig::updateData(const char *json) {
    Log.info("updateData called %s", json);
    updateDataStatus = UpdateDataStatus::SUCCESS;

    if (storageMethod) {
        storageMethod->updateData(json);

        if (dataCallback) {
            // Send notification if enabled and we have data
            dataCallback();
        }
    }

    return true;
}


void CloudConfig::updateDataFailed() {
    updateDataStatus = UpdateDataStatus::FAILURE;
}


void CloudConfigUpdateFunction::setup() {
    Particle.function(name, &CloudConfigUpdateFunction::functionHandler, this);
}


int CloudConfigUpdateFunction::functionHandler(String param) {
    CloudConfig::instance().updateData(param);
    return 0;
}


void CloudConfigUpdateSubscription::setup() {
    Particle.subscribe(eventName, &CloudConfigUpdateSubscription::subscriptionHandler, this);
}

void CloudConfigUpdateSubscription::subscriptionHandler(const char *eventName, const char *eventData) {
    CloudConfig::instance().updateData(eventData);
}

//
// CloudConfigUpdateWebhook
//
CloudConfigUpdateWebhook::CloudConfigUpdateWebhook(const char *eventName) {
    // This is the request event name
    requestEventName = eventName;

    // This is the subscription event name (hook-response)
    withEventName(eventName);
}

CloudConfigUpdateWebhook &CloudConfigUpdateWebhook::withEventName(const char *eventName) {
    // Response Template:
    // {{PARTICLE_DEVICE_ID}}/hook-response/{{PARTICLE_EVENT_NAME}}
    String s = String::format("%s/hook-response/%s/", System.deviceID().c_str(), eventName);

    CloudConfigUpdateSubscription::withEventName(s);
    return *this;
}

void CloudConfigUpdateWebhook::startUpdate() {
    Log.info("CloudConfigUpdateWebhook::startUpdate %s", requestEventName.c_str());

    Particle.publish(requestEventName, "");
}





#ifndef __CLOUDCONFIGRK_H
#define __CLOUDCONFIGRK_H

#include "Particle.h"

struct CloudConfigDataHeader {
    /**
     * @brief Magic bytes, CloudConfig::DATA_MAGIC
     * 
     * This value (0x7251dd53) is used to see if the structure has been initialized
     */
    uint32_t    magic;

    /**
     * @brief Size of this header, sizeof(CloudConfigDataHeader)
     */
    uint8_t    headerSize;

    /**
     * @brief Flag bits, not currently used. Currently 0.
     */
    uint8_t    flags;

    /**
     * @brief Size of this data after the header (not including the size of the header)
     * 
     * Used to detect when it changes to invalidate the old version. When using the
     * CloudConfigData template, this is SIZE.
     */
    uint16_t    dataSize;

    /**
     * @brief Last time the data was checked from Time.now() (seconds past January 1, 1970, UTC).
     */
    long        lastCheck;

    /**
     * @brief Reserved for future use, currently 0.
     */
    uint32_t    reserved2;

    /**
     * @brief Reserved for future use, currently 0.
     */
    uint32_t    reserved1;

    /**
     * Data goes after this header
     */
};

/**
 * @brief Structure containing both the data and a compile-time set number of bytes of space to 
 * reserve for configuration JSON data
 */
template<size_t SIZE>
struct CloudConfigData {
    CloudConfigDataHeader header;
    char jsonData[SIZE];
};

/**
 * @brief Abstract base class of all storage methods
 * 
 * All storage methods are subclasses of either CloudStorage directly (CloudStorageStatic)
 * or of CloudConfigStorageData (everything else, like CloudStorageEEPROM, CloudStorageRetained,
 * or CloudStorageFile).
 */
class CloudConfigStorage {
public:
    CloudConfigStorage() {};
    virtual ~CloudConfigStorage() {};

    bool hasJsonData() const { return getJsonData()[0] != 0; };

    virtual CloudConfigDataHeader *getDataHeader() { return 0; };
    virtual const char * const getJsonData() const = 0;

    virtual void setup() = 0;
    virtual void parse() { jsonObj = JSONValue::parseCopy(getJsonData()); };
    virtual void loop() {};

    JSONValue getJSONValue() { return jsonObj; };

    JSONValue getJSONValueForKey(const char *key) { return getJSONValueForKey(jsonObj, key); }; 

    static JSONValue getJSONValueForKey(JSONValue parentObj, const char *key); 

    JSONValue getJSONValueAtIndex(size_t index) { return getJSONValueAtIndex(jsonObj, index); }; 

    static JSONValue getJSONValueAtIndex(JSONValue parentObj, size_t index); 

    virtual bool updateData(const char *json) { return false; };

protected:
    JSONValue jsonObj;
};

/**
 * @brief Abstract base class of all storage methods that use a CloudConfigData structure
 */
class CloudConfigStorageData : public CloudConfigStorage {
public:
    CloudConfigStorageData();

    CloudConfigStorageData(CloudConfigDataHeader *header, size_t dataSize);
    virtual ~CloudConfigStorageData();

    CloudConfigStorageData &withData(CloudConfigDataHeader *header, size_t dataSize);

    virtual CloudConfigDataHeader *getDataHeader() { return header; };

    void validate();

    const char * const getJsonData() const;

    char *getJsonData();

    virtual bool updateData(const char *json);

    virtual bool save() = 0;

protected:
    CloudConfigDataHeader *header;
    size_t dataSize;
};


/**
 * @brief Abstract base class for updating cloud config data
 */
class CloudConfigUpdate {
public:
    CloudConfigUpdate() {};
    virtual ~CloudConfigUpdate() {};

    virtual void setup() {};
    virtual void loop() {};

    virtual void startUpdate() {};

    unsigned long waitAfterCloudConnectedMs = 2000;
    unsigned long updateTimeoutMs = 60000;
};

/**
 * @brief Generic base class used by all storage methods
 * 
 * You can't instantiate one of these, you need to instantiate a specific subclass such as 
 * CloudConfigEEPROM, CloudConfigRetained, or CloudConfigNoStorage.
 */
class CloudConfig {
public:
    /**
     * @brief Magic bytes used to detect if EEPROM or retained memory has been initialized
     */
    static const uint32_t DATA_MAGIC = 0x7251dd53;

    static const long UPDATE_ONCE = 0;
    static const long UPDATE_AT_RESTART = -1;

    enum class UpdateDataStatus {
        IDLE,
        IN_PROGRESS,
        SUCCESS,
        FAILURE,
        TIMEOUT
    };

    /**
     * @brief Get the singleton instance of this class
     * 
     * You always use CloudConfig::instance() to get a reference to the singleton instance of this
     * class. You cannot construct one of these as a global or on the stack. The singleton instance
     * will never be deleted.
     */
    static CloudConfig &instance();



    CloudConfig &withStorageMethod(CloudConfigStorage *storageMethod) { this->storageMethod = storageMethod; return *this; };

    CloudConfig &withUpdateMethod(CloudConfigUpdate *updateMethod) { this->updateMethod = updateMethod; return *this; };

    CloudConfig &withUpdateFrequency(long updateFrequency) { this->updateFrequency = updateFrequency; return *this; };

    CloudConfig &withUpdateFrequency(std::chrono::seconds chronoLiteral) { this->updateFrequency = chronoLiteral.count(); return *this; };

    CloudConfig &withUpdateFrequencyOnce() { this->updateFrequency = UPDATE_ONCE; return *this; };

    CloudConfig &withUpdateFrequencyAtRestart() { this->updateFrequency = UPDATE_AT_RESTART; return *this; };

    CloudConfig &withDataCallback(std::function<void(void)> dataCallback) { this->dataCallback = dataCallback; return *this; };

    /**
     * @brief You must call setup() from your app's global setup after configuring it using the withXXX() methods
     * 
     */
    void setup();

    /**
     * @brief You must call this from loop on every call to loop()
     */
    void loop();

    //
    // Wrappers to access storage values

    JSONValue getJSONValue() { return storageMethod->getJSONValue(); };

    JSONValue getJSONValueForKey(const char *key) { return storageMethod->getJSONValueForKey(key); };

    JSONValue getJSONValueForKey(JSONValue parentObj, const char *key) { return storageMethod->getJSONValueForKey(parentObj, key); };

    JSONValue getJSONValueAtIndex(size_t index) { return storageMethod->getJSONValueAtIndex(index); };

    JSONValue getJSONValueAtIndex(JSONValue parentObj, size_t index) { return storageMethod->getJSONValueAtIndex(parentObj, index); };

    int getInt(const char *key) { return storageMethod->getJSONValueForKey(key).toInt(); };

    bool getBool(const char *key) { return storageMethod->getJSONValueForKey(key).toBool(); };

    double getDouble(const char *key) { return storageMethod->getJSONValueForKey(key).toDouble(); };

    const char *getString(const char *key) { return storageMethod->getJSONValueForKey(key).toString().data(); };

    virtual bool updateData(const char *json);

    void updateDataFailed();

protected:
    /**
     * @brief Constructor - You never instantiate this class directly.
     * 
     * Instead, get a singleton instance of a subclass by using CloudConfig::instance().
     */
    CloudConfig();

    /**
     * @brief This class is a singleton and never deleted
     */
    virtual ~CloudConfig();

    /**
     * @brief This class is not copyable
     */
    CloudConfig(const CloudConfig&) = delete;

    /**
     * @brief This class is not copyable
     */
    CloudConfig& operator=(const CloudConfig&) = delete;


    void stateStart();
    void stateWaitCloudConnected();
    void stateWaitAfterCloudConnected();
    void stateWaitToUpdate();
    void stateStartUpdate();
    void stateWaitUpdateComplete();

protected:
    CloudConfigStorage *storageMethod = 0;
    CloudConfigUpdate *updateMethod = 0;
    std::function<void(void)> dataCallback = 0;
    long updateFrequency = 0;
    UpdateDataStatus updateDataStatus = UpdateDataStatus::IDLE;
    std::function<void(CloudConfig &)> stateHandler = 0;
    unsigned long stateTime = 0;
    static CloudConfig *_instance;
};

/**
 * @brief Storage method that doesn't use the cloud at all and instead has the configuration as a static string in code
 * 
 * This is mainly so you can use the same code base for cloud or local storage, swappable at compile time.
 * It's a bit of overkill for normal use.
 */
class CloudConfigStorageStatic : public CloudConfigStorage {
public:
    CloudConfigStorageStatic(const char * const jsonData) : jsonData(jsonData) {};
    virtual ~CloudConfigStorageStatic() {};

    virtual void setup() { parse(); };

    const char * const getJsonData() const { return jsonData; };

protected:
    const char * const jsonData;
};


class CloudConfigStorageRetained : public CloudConfigStorageData {
public:
    CloudConfigStorageRetained(void *retainedData, size_t totalSize) : CloudConfigStorageData((CloudConfigDataHeader *)retainedData, totalSize - sizeof(CloudConfigDataHeader)) {};
    virtual ~CloudConfigStorageRetained() {};

    virtual void setup() { parse(); };
    virtual bool save() { return true; };

};



template<size_t SIZE>
class CloudConfigStorageEEPROM : public CloudConfigStorageData {
public:
    CloudConfigStorageEEPROM(size_t eepromOffset) : CloudConfigStorageData(&dataBuffer.header, SIZE) {};
    virtual ~CloudConfigStorageEEPROM() {};

    virtual void setup() { EEPROM.get(eepromOffset, dataBuffer); parse(); };
    virtual bool save() { EEPROM.put(eepromOffset, dataBuffer); return true; };

protected:
    size_t eepromOffset;
    CloudConfigData<SIZE> dataBuffer;
};

#if HAL_PLATFORM_FILESYSTEM
#include <fcntl.h>
#include <sys/stat.h>

template<size_t SIZE>
class CloudConfigStorageFile : public CloudConfigStorageData {
public:
    CloudConfigStorageFile(const char *path) : path(path), CloudConfigStorageData(&dataBuffer.header, SIZE) {};
    virtual ~CloudConfigStorageFile() {};

    virtual void setup() {
        int fd = open(path, O_RDWR | O_CREAT);
        if (fd != -1) {
            int count = read(fd, &dataBuffer, sizeof(dataBuffer));
            if (count != sizeof(dataBuffer)) {
                Log.info("resetting file contents");

                // File contents do not appear to be valid; do not use
                memset(&dataBuffer, 0, sizeof(dataBuffer));
            }
            close(fd);   
        }
        validate();
        parse();
    }

    virtual bool save() {
        // Save to file
        Log.info("save to file");
        int fd = open(path, O_RDWR | O_CREAT);
        if (fd != -1) {
            write(fd, &dataBuffer, sizeof(dataBuffer));
            close(fd);   
            Log.info("save to file done");
        }
        return true;
    }

protected:
    String path;
    CloudConfigData<SIZE> dataBuffer;
};
#endif /* HAL_PLATFORM_FILESYSTEM */

/**
 * @brief Update configuration via Particle function call
 * 
 * Function update is a good choice if:
 * - You will be pushing the changes from your own server. 
 * - Each device has its own configuration.
 * - You want confirmation that the device received the update.
 * - The device may be in sleep mode or offline.
 * - You are using unclaimed product devices (also works if claimed).
 */
class CloudConfigUpdateFunction : public CloudConfigUpdate {
public:
    CloudConfigUpdateFunction() {};
    virtual ~CloudConfigUpdateFunction() {};

    CloudConfigUpdateFunction(const char *name) : name(name) {};

    CloudConfigUpdateFunction &withName(const char *name) { this->name = name; return *this; };

    virtual void setup();

    int functionHandler(String param);

protected:
    String name;
};

/**
 * @brief Update configuration via Particle event
 * 
 * Subscription is a good choice if:
 * - You want to update all devices at once efficiently.
 * - Devices are generally always online.
 * - Devices must be claimed to an account.
 */
class CloudConfigUpdateSubscription : public CloudConfigUpdate {
public:
    CloudConfigUpdateSubscription() {};
    virtual ~CloudConfigUpdateSubscription() {};

    CloudConfigUpdateSubscription(const char *eventName) : eventName(eventName) {};

    CloudConfigUpdateSubscription &withEventName(const char *eventName) { this->eventName = eventName; return *this; };

    virtual void setup();

    void subscriptionHandler(const char *eventName, const char *eventData);

protected:
    String eventName;
};

/**
 * @brief Update configuration via a webhook
 * 
 * This is based on the subscription model, except the device requests
 * the configuration using a webhook. You program the settings into
 * the webhook response configuration from the console, so no separate 
 * server is required.
 * 
 * - Good if you have commmon configuration across all devices
 * - Can configure settings in the console (though you have to edit JSON
 * in the webhook editor).
 * - Devices must be claimed to an account.
 */
/*
class CloudConfigUpdateWebhook : public CloudConfigUpdateSubscription {
public:
    CloudConfigUpdateWebhook();
    virtual ~CloudConfigUpdateWebhook();
};
*/

/**
 * @brief Update configuration stored in the Device Notes field 
 * 
 * This is a special case of the webhook that reads the configuration data
 * from the Device Notes field of device information.
 * 
 * - Good if you have per-device configuration.
 * - Can configure settings in the console (though you have to edit JSON).
 * - Devices must be claimed to an account.
 */
class CloudConfigUpdateDeviceNotes : public CloudConfigUpdateSubscription {
public:
    CloudConfigUpdateDeviceNotes() {};
    virtual ~CloudConfigUpdateDeviceNotes() {};

    CloudConfigUpdateDeviceNotes(const char *eventName);

    CloudConfigUpdateDeviceNotes &withEventName(const char *eventName);

    virtual void startUpdate();

protected:
    String requestEventName;
};



#endif /* __CLOUDCONFIGRK_H */

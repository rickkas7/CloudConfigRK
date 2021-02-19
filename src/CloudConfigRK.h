#ifndef __CLOUDCONFIGRK_H
#define __CLOUDCONFIGRK_H

#include "Particle.h"

/**
 * @brief Header for structure containing saved data used for retained memory, files, and EEPROM.
 * 
 * It's currently 20 bytes plus the size of the JSON configuration data that goes after it.
 */
struct CloudConfigDataHeader { // 20 bytes
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
 * 
 * @param SIZE the size of the data in bytes. The total structure will be sizeof(CloudConfigDataHeader) + SIZE
 */
template<size_t SIZE>
struct CloudConfigData {
    /**
     * @brief The data header (20 bytes) including magic bytes and version information
     */
    CloudConfigDataHeader header;

    /**
     * @brief The JSON data goes here. The maximum length is SIZE - 1 characters, as it is always null terminated.
     */
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
    /**
     * @brief Constructor. Base class constructor is empty.
     */
    CloudConfigStorage() {};

    /**
     * @brief Returns true if there is JSON data field is not empty
     */
    bool hasJsonData() const { return getJsonData()[0] != 0; };

    /**
     * @brief Gets a pointer to the data header
     * 
     * This is only available in classes derived from CloudConfigStorageData, which is all
     * of them except CloudConfigDataStatic, which is not updateable and therefore does not
     * need the structure.
     */
    virtual CloudConfigDataHeader *getDataHeader() { return 0; };

    /**
     * @brief Gets a const pointer to the JSON data
     * 
     * The CloudConfigStorageData class has an overload that can get a mutable pointer
     * so the data can be updated, but this base class does not have one, because the 
     * CloudConfigDataStatic can't update the data in program flash from the device.
     */
    virtual const char * const getJsonData() const = 0;

    /**
     * @brief Called from setup(). Optional. Only needed if the storage method wants setup processing time.
     */
    virtual void setup() = 0;

    /**
     * @brief Called after the JSON data is updated to parse the data using the JSON parser
     * 
     * You normally don't need to override this, but it is virtual in case you do.
     */
    virtual void parse() { jsonObj = JSONValue::parseCopy(getJsonData()); };

    /**
     * @brief Called from loop(). Optional. Only needed if the storage method wants loop processing time.
     */
    virtual void loop() {};

    /**
     * @brief Gets the top level (outer) JSONValue object. 
     */
    JSONValue getJSONValue() { return jsonObj; };

    /**
     * @brief Gets the value for a given key in the top-level outer JSON object
     * 
     * @param key The name of the key-value pair
     * 
     * @return A JSONValue object for that key. If the key does not exist in that object, an
     * empty JSONValue object that returns false for isValid() is returned.
     */
    JSONValue getJSONValueForKey(const char *key) { return getJSONValueForKey(jsonObj, key); }; 

    /**
     * @brief Gets the value for a given key in the a JSON object 
     * 
     * @param parentObj A JSONValue for a JSON object to look in
     * 
     * @param key The name of the key-value pair
     * 
     * @return A JSONValue object for that key. If the key does not exist in that object, an
     * empty JSONValue object that returns false for isValid() is returned.
     */
    static JSONValue getJSONValueForKey(JSONValue parentObj, const char *key); 

    /**
     * @brief Gets the value for a given index in the a JSON array in the top-level outer JSON array
     * 
     * @param index 0 = first entry, 1 = second entry, ...
     * 
     * @return A JSONValue object for that key. If the key does not exist in that object, an
     * empty JSONValue object that returns false for isValid() is returned.
     * 
     * This method is not commonly used because the top-level object is usually a JSON object
     * (surrounded by { }) not a JSON array (surrounded by [ ]).
     * 
     * There is no call to determine the length of the array; iterate until an invalid object
     * is returned.
     */
    JSONValue getJSONValueAtIndex(size_t index) { return getJSONValueAtIndex(jsonObj, index); }; 

    /**
     * @brief Gets the value for a given index in the a JSON array
     * 
     * @param parentObj A JSONValue for a JSON object to look in
     * 
     * @param index 0 = first entry, 1 = second entry, ...
     * 
     * @return A JSONValue object for that key. If the key does not exist in that object, an
     * empty JSONValue object that returns false for isValid() is returned.
     * 
     * There is no call to determine the length of the array; iterate until an invalid object
     * is returned.
     */
    static JSONValue getJSONValueAtIndex(JSONValue parentObj, size_t index); 

    /**
     * @brief This method is called to update the data in storage method and parse it again
     * 
     * @param json The new JSON data to save
     * 
     * This is subclassed in CloudConfigStorageData.
     */
    virtual bool updateData(const char *json) { return false; };

protected:
    /**
     * @brief Destructor. Base class destructor is empty. Also, you can't delete one of these.
     */
    virtual ~CloudConfigStorage() {};

    /**
     * @brief This class is not copyable
     */
    CloudConfigStorage(const CloudConfigStorage&) = delete;

    /**
     * @brief This class is not copyable
     */
    CloudConfigStorage& operator=(const CloudConfigStorage&) = delete;

    /**
     * @brief The JSONValue object for the outermost JSON object (or array)
     * 
     * The parse() method sets this. The parse() method is called from setup() and updateData().
     */
    JSONValue jsonObj;
};

/**
 * @brief Abstract base class of all storage methods that use a CloudConfigData structure
 * 
 * Classes that include a CloudConfigDataHeader (Retained, EEPROM, File) all use this class as
 * a base class as it contains handy common features that simplify those implementations.
 */
class CloudConfigStorageData : public CloudConfigStorage {
public:
    CloudConfigStorageData() {};

    CloudConfigStorageData(CloudConfigDataHeader *header, size_t dataSize);

    CloudConfigStorageData &withData(CloudConfigDataHeader *header, size_t dataSize);

    virtual CloudConfigDataHeader *getDataHeader() { return header; };

    void validate();

    const char * const getJsonData() const;

    char *getJsonData();

    virtual bool updateData(const char *json);

    /**
     * @brief Save the data in the storage subclass
     * 
     * @return true on success or false on failure. 
     */
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

    virtual void setup() {};
    virtual void loop() {};

    virtual void startUpdate() {};

    unsigned long waitAfterCloudConnectedMs = 2000;
    unsigned long updateTimeoutMs = 60000;

protected:
    /**
     * @brief Destructor. Base class destructor is empty. Also, you can't delete one of these.
     */
    virtual ~CloudConfigUpdate() {};

    /**
     * @brief This class is not copyable
     */
    CloudConfigUpdate(const CloudConfigUpdate&) = delete;

    /**
     * @brief This class is not copyable
     */
    CloudConfigUpdate& operator=(const CloudConfigUpdate&) = delete;

};

/**
 * @brief Singleton class for managing cloud-based configuration
 * 
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
    // Convenience wrappers to access storage values
    // 

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

    virtual void setup() { parse(); };

    const char * const getJsonData() const { return jsonData; };

protected:
    const char * const jsonData;
};

/**
 * @brief Storage method to store data in retained memory
 * 
 * Retained memory is preserved across restarts and across all sleep modes including HIBERNATE mode. There is around 3K of retained memory available.
 */
class CloudConfigStorageRetained : public CloudConfigStorageData {
public:
    /**
     * @brief Construct a storage method for retained RAM. 
     * 
     * @param retainedData A pointer to the retained buffer, typically a CloudConfigData<> struct.
     * 
     * @param totalSize The size of the retained data including both the header and data. It's that way so you can use sizeof() easily.
     * 
     * You typically allocate one of these as a global variable or using new from setup(). 
     * 
     * You cannot delete or copy one of these objects.
     */
    CloudConfigStorageRetained(void *retainedData, size_t totalSize) : CloudConfigStorageData((CloudConfigDataHeader *)retainedData, totalSize - sizeof(CloudConfigDataHeader)) {};
    
    virtual void setup() { validate(); };
    virtual bool save() { return true; };
};



template<size_t SIZE>
class CloudConfigStorageEEPROM : public CloudConfigStorageData {
public:
    CloudConfigStorageEEPROM(size_t eepromOffset) : CloudConfigStorageData(&dataBuffer.header, SIZE) {};

    virtual void setup() { EEPROM.get(eepromOffset, dataBuffer); validate(); };
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

    CloudConfigUpdateSubscription(const char *eventName) : eventName(eventName) {};

    CloudConfigUpdateSubscription &withEventName(const char *eventName) { this->eventName = eventName; return *this; };

    virtual void setup();

    void subscriptionHandler(const char *eventName, const char *eventData);

protected:
    String eventName;
};

/**
 * @brief Update configuration from data retrieved from a webhook
 * 
 * Two examples that use this are the Devices Notes example and Google Sheets example
 */
class CloudConfigUpdateWebhook : public CloudConfigUpdateSubscription {
public:
    CloudConfigUpdateWebhook() {};
    
    CloudConfigUpdateWebhook(const char *eventName);

    CloudConfigUpdateWebhook &withEventName(const char *eventName);

    virtual void startUpdate();

protected:
    String requestEventName;
};



#endif /* __CLOUDCONFIGRK_H */

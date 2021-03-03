#ifndef __CLOUDCONFIGRK_H
#define __CLOUDCONFIGRK_H

#include "Particle.h"

// Github repository: https://github.com/rickkas7/CloudConfigRK
// License: MIT

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
     * 
     * This is typically an object, but could be an array.
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
    /**
     * @brief Default constructor
     */
    CloudConfigStorageData() {};

    /**
     * @brief Constructor with header
     * 
     * This constructor is basically the same as using the default contructor then using withData()
     * 
     * @param header The pointer to the  CloudConfigDataHeader structure 
     * 
     * @param dataSize The size of the data after the header. The total size is sizeof(CloudConfigDataHeader) + dataSize.
     */
    CloudConfigStorageData(CloudConfigDataHeader *header, size_t dataSize);

    /**
     * @brief Specify the data header and data location
     * 
     * @param header The pointer to the  CloudConfigDataHeader structure 
     * 
     * @param dataSize The size of the data after the header. The total size is sizeof(CloudConfigDataHeader) + dataSize.
     * 
     * @return Returns *this so you can chain calls, fluent-style.
     */
    CloudConfigStorageData &withData(CloudConfigDataHeader *header, size_t dataSize);

    /**
     * @brief Gets a pointer to the CloudConfigDataHeader structure
     */
    virtual CloudConfigDataHeader *getDataHeader() { return header; };

    /**
     * @brief Validate the CloudConfigDataHeader
     * 
     * If not valid, then resets the entire structure to default values. This is common when the structure is
     * first initialized.
     */
    void validate();

    /**
     * @brief Gets a pointer to the JSON data for reading (const)
     *
     * This is right after the CloudConfigDataHeader structure.
     */
    const char * const getJsonData() const;

    /**
     * @brief Returns the maximum size of the JSON data in characters
     * 
     * The buffer is one byte larger than this because there is always a trailling null to make it
     * a c-string.
     * 
     * @return Maximum number of characters of JSON data
     */
    size_t getMaxJsonDataSize() const { return dataSize - 1; };

    /**
     * @brief Gets a pointer to the JSON data for writing (not const)
     *
     * This is right after the CloudConfigDataHeader structure. This method is not available on all storage
     * classes. For example, CloudStorageStatic stores the data in program flash, so it can't be written to.
     */
    char *getJsonData();

    /**
     * @brief Gets the total size of the buffer including both the header and the JSON data
     * 
     * This is the buffer size; the actual JSON string could be shorter as it's variable length.
     */
    size_t getTotalSize() const { return sizeof(CloudConfigDataHeader) + dataSize; };

    /**
     * @brief This method is called after subclasses update the JSON data
     * 
     * The default implementation in this class is usually sufficient, but it can be overridden
     * by subclasses if necessary.
     * 
     * - Checks to make sure json will fit in dataSize
     * - Copies the JSON data (with trailing null)
     * - Calls parse() to run the JSMN JSON parser on it and update jsonObj in the parent class
     * - Calls save() to save the data in this storage method
     */
    virtual bool updateData(const char *json);

    /**
     * @brief Save the data in the storage subclass. Must be implemented in subclasses!
     * 
     * @return true on success or false on failure. 
     */
    virtual bool save() = 0;

protected:
    /**
     * @brief Pointer to the header with data after it
     */
    CloudConfigDataHeader *header;

    /**
     * @brief Size of the JSON data, not including sizeof(CloudConfigDataHeader)
     */
    size_t dataSize;
};


/**
 * @brief Abstract base class for updating cloud config data
 */
class CloudConfigUpdate {
public:
    /**
     * @brief Default constructor
     */
    CloudConfigUpdate() {};

    /**
     * @brief Called during setup() to allow update methods to have setup time 
     * 
     * Optional for subclasses to override.
     */
    virtual void setup() {};

    /**
     * @brief Called during loop() to allow update methods to have loop time 
     * 
     * Optional for subclasses to override.
     */
    virtual void loop() {};

    /**
     * @brief Called when subclasses should update the data from the cloud.
     * 
     * Optional for subclasses to override, but typically useful to do so. It is
     * called after cloud connected and after waitAfterCloudConnectedMs depending
     * on the settings for updateFrequency.
     * 
     * The subclass must call ConfigConfig::instance().updateData() or 
     * ConfigConfig::instance().updateDataFailed() if this method is called.
     */
    virtual void startUpdate() {};

    /**
     * @brief How long to wait after cloud connected to update settings.
     */
    unsigned long waitAfterCloudConnectedMs = 2000;

    /**
     * @brief If a response is not received in this amount of time, an error is assumed
     * and another attempt will be made later.
     * 
     * If subclasses take a long time to retrieve data you may need to override this
     * value from your subclass constructor or setup method.
     */
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

    /**
     * @brief Value used for updateFrequency to only request configuration data if it has never been saved
     */
    static const long UPDATE_ONCE = 0;

    /**
     * @brief Value used for updateFrequency to request configuration data on every restart
     * 
     * Note that this also means after every HIBERNATE sleep wake!
     */
    static const long UPDATE_AT_RESTART = -1;

    /**
     * @brief Enumeration used for the status of updateData
     */
    enum class UpdateDataStatus {
        /**
         * @brief updateData is not in progress
         */
        IDLE,

        /**
         * @brief updateData is in progress (startData called, but updateData or updateDataFailed not called yet, and not at timeout)
         */
        IN_PROGRESS,

        /**
         * @brief After startData(), updateData() was called so the data was successfully retrieved.
         */
        SUCCESS,

        /**
         * @brief After startData(), updateDataFailed() was called so the data was not retrieved.
         */
        FAILURE,

        /**
         * @brief After startData(), neither updateData() nor updateDataFailed() were called by the timeout value
         */
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


    /**
     * @brief Sets the storage method class, required before setup() is called.
     * 
     * @param storageMethod The CloudConfigStorage subclass instance to use to store and retrieve the data
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * There can only be one storage method, and once set, cannot be changed or removed. 
     * 
     */
    CloudConfig &withStorageMethod(CloudConfigStorage *storageMethod) { this->storageMethod = storageMethod; return *this; };

    /**
     * @brief Sets the update method class, not required, but if used, call before setup() is called.
     * 
     * @param updateMethod The CloudConfigUpdate subclass instance to use to get data from the cloud
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * There can only be one update method, and once set, cannot be changed or removed. 
     */
    CloudConfig &withUpdateMethod(CloudConfigUpdate *updateMethod) { this->updateMethod = updateMethod; return *this; };


    /**
     * @brief Sets the update frequency in seconds
     * 
     * @param updateFrequency update frequency in seconds
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * See also the overload that takes a chrono literal, as well as withUpdateFrequencyOnce() and withUpdateFrequencyAtRestart().
     * 
     * Can be called after setup to change the update frequency.
     */
    CloudConfig &withUpdateFrequency(long updateFrequency) { this->updateFrequency = updateFrequency; return *this; };

    /**
     * @brief Sets the update frequency as a chrono literal
     * 
     * @param chronoLiteral update frequency as a chrono literal. For example: 15min (15 minutes) or 24h (24 hours).
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * See also withUpdateFrequencyOnce() and withUpdateFrequencyAtRestart().
     * 
     * Can be called after setup to change the update frequency.
     */
    CloudConfig &withUpdateFrequency(std::chrono::seconds chronoLiteral) { this->updateFrequency = chronoLiteral.count(); return *this; };

    /**
     * @brief Sets the update frequency only if there is no saved value. 
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * If there is no saved value, then the update occurs after connecting to the cloud.
     * 
     * Should be done before setup() to make sure it takes effect in time.
     */
    CloudConfig &withUpdateFrequencyOnce() { this->updateFrequency = UPDATE_ONCE; return *this; };

    /**
     * @brief Sets the update frequency to update once after cloud connecting after restart.
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * This is anything that calls setup() so includes device reset and wake from HIBERNATE sleep mode.
     * 
     * Should be done before setup() to make sure it takes effect in time.
     */
    CloudConfig &withUpdateFrequencyAtRestart() { this->updateFrequency = UPDATE_AT_RESTART; return *this; };

    /**
     * @brief Adds a callback to be called after data is loaded or updated
     * 
     * @param dataCallback the callback function to call
     * 
     * @return Returns *this so you can chain together the withXXX() methods, fluent-style.
     * 
     * Must be called before setup().
     * 
     * The method should have this prototype:
     *  
     *   void callback(void);
     * 
     * The callback can also be a C++11 lambda, which you can use to call a C++ member function, if desired.
     */
    CloudConfig &withDataCallback(std::function<void(void)> dataCallback) { this->dataCallback = dataCallback; return *this; };

    /**
     * @brief You must call setup() from your app's global setup after configuring it using the withXXX() methods
     */
    void setup();

    /**
     * @brief You must call this from loop on every call to loop()
     */
    void loop();

    //
    // Convenience wrappers to access storage values
    // 

    /**
     * @brief Gets the top level (outer) JSONValue object. 
     * 
     * This is typically an object, but could be an array.
     */
    JSONValue getJSONValue() { return storageMethod->getJSONValue(); };

    /**
     * @brief Gets the value for a given key in the top-level outer JSON object
     * 
     * @param key The name of the key-value pair
     * 
     * @return A JSONValue object for that key. If the key does not exist in that object, an
     * empty JSONValue object that returns false for isValid() is returned.
     * 
     * The getInt(), getBool(), getString() are convenience methods for
     * getJSONValueForKey(key).toInt(), etc.
     */
    JSONValue getJSONValueForKey(const char *key) { return storageMethod->getJSONValueForKey(key); };

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
    JSONValue getJSONValueForKey(JSONValue parentObj, const char *key) { return storageMethod->getJSONValueForKey(parentObj, key); };

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
    JSONValue getJSONValueAtIndex(size_t index) { return storageMethod->getJSONValueAtIndex(index); };

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
    JSONValue getJSONValueAtIndex(JSONValue parentObj, size_t index) { return storageMethod->getJSONValueAtIndex(parentObj, index); };

    /**
     * @brief Convenience method for getting a top level JSON integer value by its key name
     * 
     * @param key The JSON key to retrieve
     * 
     * If the key does not exist, 0 is returned.
     */
    int getInt(const char *key) { return storageMethod->getJSONValueForKey(key).toInt(); };

    /**
     * @brief Convenience method for getting a top level JSON integer value by its key name
     * 
     * @param key The JSON key to retrieve
     * 
     * If the key does not exist, false is returned.
     */
    bool getBool(const char *key) { return storageMethod->getJSONValueForKey(key).toBool(); };

    /**
     * @brief Convenience method for getting a top level JSON integer value by its key name
     * 
     * @param key The JSON key to retrieve
     * 
     * If the key does not exist, 0 is returned.
     */
    double getDouble(const char *key) { return storageMethod->getJSONValueForKey(key).toDouble(); };

    /**
     * @brief Convenience method for getting a top level JSON integer value by its key name
     * 
     * @param key The JSON key to retrieve
     * 
     * If the key does not exist, an empty string is returned.
     */
    const char *getString(const char *key) { return storageMethod->getJSONValueForKey(key).toString().data(); };

    /**
     * @brief Update methods call this when they have JSON configuration data
     * 
     * The maximum size of the data is dependent on the storage method and how it was configured. 
     */
    virtual bool updateData(const char *json);

    /**
     * @brief Update methods call this if they tried but failed to get configuration data
     * 
     * If you don't call updateData() or updateDataFailed() from the update method, then eventually it will
     * time out, which will be treated as a failure.
     */
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

    /**
     * @brief State handler this is the first state entered
     * 
     * May call dataCallback if there is valid saved data
     * 
     * Next state: stateWaitCloudConnected
     */
    void stateStart();

    /**
     * @brief State handler to wait for cloud connnected
     * 
     * Waits until Particle.connected() is true and also Time.isValid(). The latter is necessary
     * because the store a timestamp of when we last checked the cloud settings.
     * 
     * Previous state: stateStart
     * Next state: stateWaitAfterCloudConnected
     */
    void stateWaitCloudConnected();

    /**
     * @brief State handler for the waiting time after cloud connected
     * 
     * This waits waitAfterCloudConnectedMs, which is configurable within the update
     * method. Default is 2 seconds.
     * 
     * Previous state: stateWaitCloudConnected
     * Next state: stateStartUpdate if we need to retrieve settings
     * or stateWaitToUpdate if there is no update required (not time yet)
     */
    void stateWaitAfterCloudConnected();

    /**
     * @brief State handler for waiting to update
     * 
     * If updateFrequency is set to a periodic value greater than zero, this state
     * waits until the time has expired.
     * 
     * Previous state: stateWaitAfterCloudConnected
     * Next state: stateStartUpdate
     */
    void stateWaitToUpdate();

    /**
     * @brief State handler to have the update method actually update settings
     * 
     * The startUpdate method of the update method is called in this state.
     * 
     * Previous state: stateWaitAfterCloudConnected or stateWaitToUpdate
     * Next state: stateWaitUpdateComplete
     */
    void stateStartUpdate();

    /**
     * @brief Waits for updateData or updateDataFailed to be called
     * 
     * The update method starts the update process when startUpdate() is called. Then
     * they either call CloudConfig::instance().updateData() or updateDataFailed().
     * 
     * If neither is called before updateTimeoutMs expires (default: 60 seconds)
     * an error is assume to have occurred and is treated as if the updateDataFailed.
     * 
     * Previous state: stateStartUpdate
     * Next state: stateWaitToUpdate
     */
    void stateWaitUpdateComplete();

protected:
    /**
     * @brief Storage object - used to store data in retained memory, EEPROM, or flash file system file
     */
    CloudConfigStorage *storageMethod = 0;

    /**
     * @brief Update object - used to request data from the cloud
     */
    CloudConfigUpdate *updateMethod = 0;

    /**
     * @brief Callback to call when there is data
     * 
     * This is called either at startup, if data is located from the storage method and is valid, or
     * after data is updated from the cloud. For some storage methods like function or subscription, this
     * can also be called spontaneously when the cloud-side sends new configuration data.
     */
    std::function<void(void)> dataCallback = 0;

    /**
     * @brief How often to update the data from the cloud
     * 
     * Values > 0 are in seconds. There are also two special values: UPDATE_ONCE (0) and UPDATE_AT_RESTART (-1);
     * 
     * Normally you use the withUpdateFrequency() methods to set this.
     */
    long updateFrequency = 0;

    /**
     * @brief Status of updateData. Used when in stateWaitUpdateComplete.
     * 
     * The updateData() and updateDataFailed() methods change this value.
     */
    UpdateDataStatus updateDataStatus = UpdateDataStatus::IDLE;

    /**
     * @brief State handler for the main state machine
     */
    std::function<void(CloudConfig &)> stateHandler = 0;

    /**
     * @brief Time value (millis) used in some states
     */
    unsigned long stateTime = 0;

    /**
     * @brief Singleton instance of this class
     */
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
    /**
     * @brief Constructor
     * 
     * @param jsonData The JSON data to use as the configuration data (string constant)
     */
    CloudConfigStorageStatic(const char * const jsonData) : jsonData(jsonData) {};

    /**
     * @brief Called during setup() to parse the JSON data
     */
    virtual void setup() { parse(); };

    /**
     * @brief Return the static JSON data
     */
    const char * const getJsonData() const { return jsonData; };

protected:
    /**
     * @brief The value passed to the constructor
     */
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
    
    /**
     * @brief Called during setup() to validate and parse the JSON data
     */
    virtual void setup() { validate(); };

    /**
     * @brief Retained data doesn't require an explict save
     */
    virtual bool save() { return true; };
};


/**
 * @brief Storage method to store data in the emulated EEPROM
 * 
 * This is a good choice for storing data on Gen 2 devices if you have available space in EEPROM. 
 * 
 * On Gen 3 devices (Argon, Boron, B  Series SoM, Tracker SoM) the file system is usually a better choice.
 * Since the emulated EEPROM is just a file on the file system on Gen 3, there is no performance
 * advantage for using EEPROM over File.
 * 
 * @param SIZE the templated maximum size of the JSON data. 
 * 
 * The getTotalSize() method of the parent class determines how many bytes of EEPROM are used; it's
 * the size of the CloudConfigDataHeader plus SIZE.
 */
template<size_t SIZE>
class CloudConfigStorageEEPROM : public CloudConfigStorageData {
public:
    /**
     * @brief Constructor for EEPROM storage
     * 
     * @param eepromOffset Starting offset in EEPROM to store the data. 
     * 
     * The getTotalSize() method of the parent class determines how many bytes of EEPROM are used; it's
     * the size of the CloudConfigDataHeader plus SIZE.
     */
    CloudConfigStorageEEPROM(size_t eepromOffset) : CloudConfigStorageData(&dataBuffer.header, SIZE), eepromOffset(eepromOffset) {};

    /**
     * @brief Called during setup() to load, validate, and parse the JSON data
     */
    virtual void setup() { EEPROM.get(eepromOffset, dataBuffer); validate(); };

    /**
     * @brief Called to save the data in EEPROM when it is updated
     */
    virtual bool save() { EEPROM.put(eepromOffset, dataBuffer); return true; };

protected:
    /**
     * @brief Offset into the emulated EEPROM to start storing the data.
     */
    size_t eepromOffset;

    /**
     * @brief Copy of the EEPROM data in RAM
     */
    CloudConfigData<SIZE> dataBuffer;
};

#if HAL_PLATFORM_FILESYSTEM
#include <fcntl.h>
#include <sys/stat.h>

/**
 * @brief Storage method to store data in the flash file system
 * 
 * On Gen 3 devices (Argon, Boron, B  Series SoM, Tracker SoM) the file system is usually a good choice.
 * Since the emulated EEPROM is just a file on the file system on Gen 3, there is no performance
 * advantage for using EEPROM over File.
 * 
 * The file system is 2 MB on most devices, 4 MB on the Tracker.
 * 
 * @param SIZE the templated maximum size of the JSON data. 
 * 
 * You need for SIZE to be larger enough for the largest configuration you will have, but it does
 * reserve sizeof(CloudConfigDataHeader) + SIZE bytes of RAM, so you don't want it be excessively
 * large. For function, subscription and webhook update methods, SIZE can't be larger than 622 bytes.
 */
template<size_t SIZE>
class CloudConfigStorageFile : public CloudConfigStorageData {
public:
    /**
     * @brief Constructor for storage of data in a flash file system file
     * 
     * @param path The pathname (slash separated, like Unix) to the file to store the data in. 
     */
    CloudConfigStorageFile(const char *path) : path(path), CloudConfigStorageData(&dataBuffer.header, SIZE) {};

    /**
     * @brief Called during setup() to load, validate, and parse the JSON data
     */
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

    /**
     * @brief Called to save the data to the file when it is updated
     */
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
    /**
     * @brief The file system path passed to the contructor
     */
    String path;

    /**
     * @brief Copy of the file data in RAM
     */
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
    /**
     * @brief Default constructor
     */
    CloudConfigUpdateFunction() {};

    /**
     * @brief Constructor with function name
     * 
     * @param name The name of the function to register
     */
    CloudConfigUpdateFunction(const char *name) : name(name) {};

    /**
     * @brief Method to set the name of the Particle.function to register
     * 
     * @param name The name of the function to register
     * 
     * This must be called before setup(). Calling it later will have no effect.
     */
    CloudConfigUpdateFunction &withName(const char *name) { this->name = name; return *this; };

    /**
     * @brief Called during setup()
     * 
     * Be sure to set the name before calling setup()!
     */
    virtual void setup();

    /**
     * @brief Function that is called when the Particle.function is called.
     */
    int functionHandler(String param);

protected:
    /**
     * @brief Function name passed to constructor or withName. Used during setup().
     */
    String name;
};

/**
 * @brief Update configuration via Particle event
 * 
 * Subscription is a good choice if:
 * - You want to update all devices at once efficiently.
 * - Devices are generally always online.
 * - Devices must be claimed to an account.
 * 
 * You will probably want to subclass this if you want to be able to have the device request an update
 * since there is no way for this class to know how to request it. See CloudConfigUpdateWebhook for
 * and example of using startUpdate() to make a request.
 */
class CloudConfigUpdateSubscription : public CloudConfigUpdate {
public:
    /**
     * @brief Default constructor
     */
    CloudConfigUpdateSubscription() {};

    /**
     * @brief Constructor with event name
     * 
     * @param eventName The name of the event to subscribe to
     */
    CloudConfigUpdateSubscription(const char *eventName) : eventName(eventName) {};

    /**
     * @brief Method to set the event name to subscribe to
     * 
     * @param eventName The name of the event to subscribe to
     * 
     * This must be called before setup(). Calling it later will have no effect.
     */
    CloudConfigUpdateSubscription &withEventName(const char *eventName) { this->eventName = eventName; return *this; };

    /**
     * @brief Called during setup()
     * 
     * Be sure to set the eventName before calling setup()!
     */
    virtual void setup();

    /**
     * @brief Handler called when the event is received
     */
    void subscriptionHandler(const char *eventName, const char *eventData);

protected:
    /**
     * @brief Subscribed event name passed to constructor or withEventName. Used during setup().
     */
    String eventName;
};

/**
 * @brief Update configuration from data retrieved from a webhook
 * 
 * Two examples that use this are the Devices Notes example and Google Sheets example.
 * 
 * This is derived from CloudConfigUpdateSubscription but is different because it subscribes
 * to a webhook response event, not eventName.
 */
class CloudConfigUpdateWebhook : public CloudConfigUpdateSubscription {
public:
    /**
     * @brief Default constructor
     */
    CloudConfigUpdateWebhook() {};
    
    /**
     * @brief Constructor with event name
     * 
     * @param eventName The name of the event of the webhook
     * 
     * Note that eventName should be the event name, not the hook-response event that is actually
     * subscribed to.
     */
    CloudConfigUpdateWebhook(const char *eventName);

    /**
     * @brief Method to set the event name to for the webhook
     * 
     * @param eventName The name of the event of the webhook
     * 
     * This must be called before setup(). Calling it later will have no effect.
     * 
     * Note that eventName should be the event name, not the hook-response event that is actually
     * subscribed to.
     */
    CloudConfigUpdateWebhook &withEventName(const char *eventName);

    /**
     * @brief Called when a JSON data update is requested
     */
    virtual void startUpdate();

protected:
    /**
     * @brief Event name passed to constructor or withEventName. Used during setup().
     * 
     * This is not the hook-response event name!
     */
    String requestEventName;
};



#endif /* __CLOUDCONFIGRK_H */

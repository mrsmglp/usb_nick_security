#include "device_manager.h"
#include <gcrypt.h>
#include <iomanip>
#include <libudev.h>
#include <sstream>
#include <iostream>
#include <fstream>

std::string DeviceManager::computeDeviceHash(const std::string& deviceIdentifier) {
    gcry_md_hd_t hd;
    const unsigned char* hashData = reinterpret_cast<const unsigned char*>(deviceIdentifier.c_str());
    unsigned char hash[gcry_md_get_algo_dlen(GCRY_MD_SHA512)];
    gcry_md_open(&hd, GCRY_MD_SHA512, GCRY_MD_FLAG_SECURE);
    gcry_md_write(hd, hashData, deviceIdentifier.size());
    gcry_md_final(hd);
    memcpy(hash, gcry_md_read(hd, 0), gcry_md_get_algo_dlen(GCRY_MD_SHA512));
    gcry_md_close(hd);

    std::ostringstream oss;
    for (size_t i = 0; i < gcry_md_get_algo_dlen(GCRY_MD_SHA512); ++i) {
        oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)hash[i];
    }
    return oss.str();
}

std::string DeviceManager::addDeviceHash(struct udev_device* dev) {
    std::ostringstream deviceIdentifier;
    const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
    const char* product = udev_device_get_sysattr_value(dev, "idProduct");
    const char* serial = udev_device_get_sysattr_value(dev, "serial");
    const char* subsystem = udev_device_get_subsystem(dev);
    const char* driver = udev_device_get_driver(dev);

    bool hasAttributes = false;

    if (vendor) {
        deviceIdentifier << vendor;
        hasAttributes = true;
    } else {
        deviceIdentifier << "Unknown";
    }
    deviceIdentifier << ":";

    if (product) {
        deviceIdentifier << product;
        hasAttributes = true;
    } else {
        deviceIdentifier << "Unknown";
    }
    deviceIdentifier << ":";

    if (serial) {
        deviceIdentifier << serial;
        hasAttributes = true;
    } else {
        deviceIdentifier << "Unknown";
    }
    deviceIdentifier << ":";

    if (subsystem) {
        deviceIdentifier << subsystem;
        hasAttributes = true;
    } else {
        deviceIdentifier << "Unknown";
    }
    deviceIdentifier << ":";

    if (driver) {
        deviceIdentifier << driver;
        hasAttributes = true;
    } else {
        deviceIdentifier << "Unknown";
    }

    if (!hasAttributes) {
        std::cerr << "Failed to retrieve necessary attributes for computing device hash." << std::endl;
        return "";
    }

    std::string deviceIdentifierStr = deviceIdentifier.str();
    std::string calculatedHash = computeDeviceHash(deviceIdentifierStr);
    deviceHashes[deviceIdentifierStr] = calculatedHash;

    return calculatedHash;
}

std::string DeviceManager::getDeviceHash(const std::string& deviceID) {
    if (deviceHashes.find(deviceID) != deviceHashes.end()) {
        return deviceHashes[deviceID];
    } else {
        return "";
    }
}

void DeviceManager::removeDeviceHash(const std::string& deviceID) {
    deviceHashes.erase(deviceID);
}

void DeviceManager::saveHashesToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    for (const auto& pair : deviceHashes) {
        outFile << pair.first << " " << pair.second << std::endl;
    }
    outFile.close();
}

void DeviceManager::loadHashesFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    std::string deviceIdentifier, hash;
    while (inFile >> deviceIdentifier >> hash) {
        deviceHashes[deviceIdentifier] = hash;
    }
    inFile.close();
}

bool DeviceManager::isDeviceTrusted(const std::string& deviceInfo) {
    std::string hash = computeDeviceHash(deviceInfo);
    return whitelist.find(hash) != whitelist.end();
}

bool DeviceManager::isDeviceBlocked(const std::string& deviceInfo) {
    std::string hash = computeDeviceHash(deviceInfo);
    return blacklist.find(hash) != blacklist.end();
}

void DeviceManager::addToWhitelist(const std::string& deviceHash) {
    whitelist.insert(deviceHash);
}

void DeviceManager::addToBlacklist(const std::string& deviceHash) {
    blacklist.insert(deviceHash);
}
    
void DeviceManager::removeFromWhitelist(const std::string& deviceHash) {
    whitelist.erase(deviceHash);
}

void DeviceManager::removeFromBlacklist(const std::string& deviceHash) {
    blacklist.erase(deviceHash);
}

void DeviceManager::loadWhitelistFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    std::string hash;
    while (inFile >> hash) {
        whitelist.insert(hash);
    }
    inFile.close();
}

void DeviceManager::loadBlacklistFromFile(const std::string& filename) {
    std::ifstream inFile(filename);
    std::string hash;
    while (inFile >> hash) {
        blacklist.insert(hash);
    }
    inFile.close();
}

void DeviceManager::saveWhitelistToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    for (const auto& hash : whitelist) {
        outFile << hash << std::endl;
    }
    outFile.close();
}

void DeviceManager::saveBlacklistToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    for (const auto& hash : blacklist) {
        outFile << hash << std::endl;
    }
    outFile.close();
}

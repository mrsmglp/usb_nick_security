#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <atomic>

class DeviceManager {
private:
    // Контейнер для хранения хэш-сумм устройств
    std::unordered_map<std::string, std::string> deviceHashes;

    // Вычисление хэш устройства
    std::string computeDeviceHash(const std::string& deviceIdentifier);

    // Белые и черные списки
    std::unordered_set<std::string> whitelist;
    std::unordered_set<std::string> blacklist;

    std::atomic<bool> stopMonitoring;

public:

    DeviceManager() : stopMonitoring(false) {}

    void setStopMonitoring(bool value) { stopMonitoring = value; }
    bool shouldStopMonitoring() const { return stopMonitoring; }

    // Добавить хэш устройства
    std::string addDeviceHash(struct udev_device* dev);

    // Получить хэш устройства по идентификатору
    std::string getDeviceHash(const std::string& deviceID);

    // Удалить хэш устройства
    void removeDeviceHash(const std::string& deviceID);

    // Сохранение и загрузка хэшей
    void saveHashesToFile(const std::string& filename);
    void loadHashesFromFile(const std::string& filename);

    // Для черных и белых списков
    bool isDeviceTrusted(const std::string& deviceInfo);
    bool isDeviceBlocked(const std::string& deviceInfo);
    void addToWhitelist(const std::string& deviceHash);
    void addToBlacklist(const std::string& deviceHash);
    void removeFromWhitelist(const std::string& deviceHash);
    void removeFromBlacklist(const std::string& deviceHash);
    void loadWhitelistFromFile(const std::string& filename);
    void loadBlacklistFromFile(const std::string& filename);
    void saveWhitelistToFile(const std::string& filename);
    void saveBlacklistToFile(const std::string& filename);
};

#endif // DEVICE_MANAGER_H

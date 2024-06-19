#ifndef USB_MONITOR_H
#define USB_MONITOR_H

#include <libudev.h>
#include "device_manager.h"

// СОздание монитора udev
void monitorDevices(DeviceManager& deviceManager);

// Показать информацию о USB-устройстве
void showUSBDeviceInfo(struct udev_device* dev, DeviceManager& deviceManager);

// Показать все устройства
void showAllDevices(DeviceManager& deviceManager);

// Очистка экрана
void clearScreen();

// Получить расположение исполняемого файла программы
std::string getProgramPath();

// Проверка существования файла правил udev
bool udevRulesFileExists(const std::string& filePath);

// Открытие файла в текстовом редакторе
void openFileInEditor(const std::string& filePath);

// Перезагрузка правил udev
void reloadUdevRules();

// Генерация правила
std::string generateRule(const std::string& action);

// Добавление правила
void addRule(const std::string& rule, const std::string& prefix);

// Добавление правила Allow
void addAllowRule();

// Добавление правила Block
void addBlockRule();

// Добавление правила Reject
void addRejectRule();

#endif // USB_MONITOR_H

#include "logger.h"
#include "usb_monitor.h"
#include "user_management.h"
#include "config.h"
#include <cstring>
#include <iostream>
#include <linux/limits.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

extern DeviceManager deviceManager;

void monitorDevices(DeviceManager& deviceManager) {
    struct udev* udev = udev_new();
    if (!udev) {
        std::cerr << "\nCan't create udev" << std::endl;
        return;
    }

    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        std::cerr << "\nCan't create udev monitor" << std::endl;
        udev_unref(udev);
        return;
    }

    udev_monitor_enable_receiving(mon);

    int fd = udev_monitor_get_fd(mon);

    while (!deviceManager.shouldStopMonitoring()) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ret = select(fd + 1, &fds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            struct udev_device* dev = udev_monitor_receive_device(mon);
            if (dev) {
                const char* action = udev_device_get_action(dev);
                if (strcmp(action, "add") == 0) {
                    std::string deviceHash = deviceManager.addDeviceHash(dev);
                    if (!deviceHash.empty()) {
                        //std::cout << "\nAdded device with hash: " << deviceHash << std::endl;
                        if (deviceManager.isDeviceTrusted(deviceHash)) {
                            //std::cout << "\nDevice is trusted." << std::endl;
                        } else if (deviceManager.isDeviceBlocked(deviceHash)) {
                            //std::cout << "\nDevice is blocked." << std::endl;
                        } else {
                            //std::cout << "\nDevice is neither trusted nor blocked." << std::endl;
                        }
                    }
                }
                udev_device_unref(dev);
            }
        }
    }

    udev_monitor_unref(mon);
    udev_unref(udev);
}

void showUSBDeviceInfo(struct udev_device* dev, DeviceManager& deviceManager) {
    const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
    const char* product = udev_device_get_sysattr_value(dev, "idProduct");
    const char* serial = udev_device_get_sysattr_value(dev, "serial");
    const char* product_name = udev_device_get_sysattr_value(dev, "product");
    const char* subsystem = udev_device_get_subsystem(dev);
    const char* driver = udev_device_get_driver(dev);
    const char* kernel = udev_device_get_devnode(dev);

    std::string vendorStr = vendor ? vendor : "Unknown";
    std::string productStr = product ? product : "Unknown";
    std::string serialStr = serial ? serial : "Unknown";
    std::string productNameStr = product_name ? product_name : "Unknown";
    std::string subsystemStr = subsystem ? subsystem : "Unknown";
    std::string driverStr = driver ? driver : "Unknown";
    std::string kernelStr = kernel ? kernel : "Unknown";

    std::cout << "\nDevice info:\n"
              << "  Vendor ID: " << vendorStr << "\n"
              << "  Product ID: " << productStr << "\n"
              << "  Serial Number: " << serialStr << "\n"
              << "  Product Name: " << productNameStr << "\n"
              << "  Subsystem: " << subsystemStr << "\n"
              << "  Driver: " << driverStr << "\n"
              << "  Kernel: " << kernelStr << "\n";

    std::string deviceIdentifier = vendorStr + ":" + productStr + ":" + serialStr + ":" + subsystemStr + ":" + driverStr;
    std::string calculatedHash = deviceManager.getDeviceHash(deviceIdentifier);

    if (!calculatedHash.empty()) {
        std::cout << "  Hash Device: " << calculatedHash << std::endl;
        if (deviceManager.isDeviceTrusted(deviceIdentifier)) {
            std::cout << "  Status: Trusted" << std::endl;
        } else if (deviceManager.isDeviceBlocked(deviceIdentifier)) {
            std::cout << "  Status: Blocked" << std::endl;
        } else {
            std::cout << "  Status: Unknown" << std::endl;
        }
    } else {
        std::cout << "  Failed to retrieve hash for the device." << std::endl;
    }
}

void showAllDevices(DeviceManager& deviceManager) {
    struct udev* udev = udev_new();
    if (!udev) {
        std::cerr << "Failed to create udev context" << std::endl;
        return;
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        std::cerr << "Failed to create udev enumerate" << std::endl;
        udev_unref(udev);
        return;
    }

    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_add_match_subsystem(enumerate, "hid");

    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
        const char* path = udev_list_entry_get_name(entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);
        if (dev) {
            std::string calculatedHash = deviceManager.addDeviceHash(dev);

            const char* vendor = udev_device_get_sysattr_value(dev, "idVendor");
            const char* product = udev_device_get_sysattr_value(dev, "idProduct");
            const char* serial = udev_device_get_sysattr_value(dev, "serial");
            const char* product_name = udev_device_get_sysattr_value(dev, "product");

            bool hasRequiredAttribute = (vendor && std::strlen(vendor) > 0) ||
                                        (product && std::strlen(product) > 0) ||
                                        (serial && std::strlen(serial) > 0) ||
                                        (product_name && std::strlen(product_name) > 0);

            if (!calculatedHash.empty() && hasRequiredAttribute) {
                showUSBDeviceInfo(dev, deviceManager);
            }

            udev_device_unref(dev);
        }
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);
}

void clearScreen() {
    std::system("clear");
}

std::string getProgramPath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
}

bool udevRulesFileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    if (file.good()) {
        std::cout << "\nUdev rules file exists." << std::endl;
        file.close();
        return true;
    } else {
        std::cout << "\nThe udev rules file does not exist or cannot be opened!" << std::endl;
        std::cout << "\nAttempting to create the file" << std::endl;
        std::ofstream outFile(filePath);
        if (outFile.is_open()) {
            outFile << "# Custom udev rules" << std::endl;
            outFile.close();
            std::cout << "\nThe udev rules file has been created successfully" << std::endl;
            return true;
        } else {
            std::cerr << "\nFailed to create udev rules file" << std::endl;
            return false;
        }
    }
}

std::string readFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logEvent({getCurrentUser(), "Failed to open file for editing:", filePath});
        return "";
    }

    // Проверка на пустой файл
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0) {
        logEvent({getCurrentUser(), "File is empty:", filePath});
        file.close();
        return "";
    }
    file.seekg(0, std::ios::beg);

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Проверка на слишком большой размер файла
    if (buffer.str().size() > 1000000) { // Например, 1MB
        logEvent({getCurrentUser(), "File is too large:", filePath});
        return "File is too large to display.";
    }

    return buffer.str();
}

void openFileInEditor(const std::string& filePath) {
    // Чтение содержимого файла перед редактированием
    std::string beforeContent = readFileContent(filePath);
    if (beforeContent.empty()) {
        logEvent({getCurrentUser(), "opened file for editing:", filePath, "file before editing is empty"});
    } else {
        logEvent({getCurrentUser(), "opened file for editing:", filePath, "file before editing:\n", beforeContent});
    }

    const std::string editor = "nano";
    std::string command = "sudo " + editor + " " + filePath;
    int ret = system(command.c_str());
    if (ret != 0) {
        logEvent({getCurrentUser(), "failed to edit file with editor:", filePath});
        std::cerr << "Failed to edit file with editor" << std::endl;
        return;
    }

    // Чтение содержимого файла после редактирования
    std::string afterContent = readFileContent(filePath);
    if (afterContent.empty()) {
        logEvent({getCurrentUser(), "closed file after editing:", filePath, "file after editing is empty"});
    } else {
        logEvent({getCurrentUser(), "closed file after editing:", filePath, "file after editing:\n", afterContent});
    }

    reloadUdevRules();
}

void reloadUdevRules() {
    std::string command = "sudo udevadm control --reload-rules && sudo udevadm trigger";
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "\nFailed to reload udev rules. Command exited with status: " << result << std::endl;
    } else {
        std::cout << greencolor << "\nUdev rules reloaded successfully." << resetcolor << std::endl;
    }
}

std::string generateRule(const std::string& action) {
    std::string vendor, product, serial, subsystem, kernel, driver;
    std::string rule = "ACTION==\"" + action + "\"";

    std::cout << "Enter subsystem (leave empty to skip): ";
    std::getline(std::cin, subsystem);
    if (!subsystem.empty()) {
        rule += ", SUBSYSTEM==\"" + subsystem + "\"";
    }

    std::cout << "Enter vendor (leave empty to skip): ";
    std::getline(std::cin, vendor);
    if (!vendor.empty()) {
        rule += ", ATTR{idVendor}==\"" + vendor + "\"";
    }

    std::cout << "Enter product (leave empty to skip): ";
    std::getline(std::cin, product);
    if (!product.empty()) {
        rule += ", ATTR{idProduct}==\"" + product + "\"";
    }

    std::cout << "Enter serial (leave empty to skip): ";
    std::getline(std::cin, serial);
    if (!serial.empty()) {
        rule += ", ATTR{serial}==\"" + serial + "\"";
    }

    std::cout << "Enter kernel (leave empty to skip): ";
    std::getline(std::cin, kernel);
    if (!kernel.empty()) {
        rule += ", KERNEL==\"" + kernel + "\"";
    }

    std::cout << "Enter driver (leave empty to skip): ";
    std::getline(std::cin, driver);
    if (!driver.empty()) {
        rule += ", DRIVERS==\"" + driver + "\"";
    }

    return rule;
}

void addRule(const std::string& rule, const std::string& prefix) {
    std::ifstream fileIn(UDEV_RULES_FILE);
    std::vector<std::string> lines;
    std::string line;
    bool exists = false;

    while (std::getline(fileIn, line)) {
        if (line.find(rule) != std::string::npos) {
            exists = true;
            break;
        }
        lines.push_back(line);
    }
    fileIn.close();

    if (exists) {
        std::cout << "\nRule already exists." << std::endl;
        return;
    }

    std::ofstream fileOut(UDEV_RULES_FILE, std::ios_base::trunc);
    if (!fileOut.is_open()) {
        std::cerr << "\nFailed to open udev rules file for writing." << std::endl;
        return;
    }

    bool isGeneralRule = (rule.find("ATTR{idVendor}") == std::string::npos &&
                          rule.find("ATTR{idProduct}") == std::string::npos &&
                          rule.find("ATTR{serial}") == std::string::npos &&
                          rule.find("KERNEL") == std::string::npos &&
                          rule.find("DRIVERS") == std::string::npos &&
                          rule.find("SUBSYSTEM") == std::string::npos);

    bool inserted = false;
    for (const auto& l : lines) {
        if (!inserted && ((isGeneralRule && l.substr(0, prefix.size()) > "00-") || (!isGeneralRule && l.substr(0, prefix.size()) > prefix))) {
            fileOut << rule << std::endl;
            inserted = true;
        }
        fileOut << l << std::endl;
    }
    if (!inserted) {
        fileOut << rule << std::endl;
    }
    fileOut.close();

    std::cout << "\nRule added successfully." << std::endl;

    std::string fullRule = rule;
    for (const auto& l : lines) {
        fullRule += "\n" + l;
    }

    logEvent({getCurrentUser(), "added new rule:", fullRule, "into file:", UDEV_RULES_FILE});
}

void addAllowRule() {
    std::string rule = generateRule("add");
    rule += ", GOTO=\"end\"";
    //rule += ", RUN+=\"/bin/sh -c 'echo 1 > /sys$env{DEVPATH}/authorized'\", GOTO=\"end\"";
    addRule(rule, "10-");
    reloadUdevRules();
}

void addBlockRule() {
    std::string rule = generateRule("add");
    rule += ", RUN+=\"/bin/sh -c 'echo 0 > /sys$env{DEVPATH}/authorized'\"";
    addRule(rule, "90-");
    reloadUdevRules();
}

void addRejectRule() {
    std::string rule = generateRule("add");
    rule += ", RUN+=\"/bin/sh -c 'echo 1 > /sys$env{DEVPATH}/remove'\"";
    addRule(rule, "90-");
    reloadUdevRules();
}

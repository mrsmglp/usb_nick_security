#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Переменные расположения особых файлов
const std::string DIRECTORY_PATH = "/etc/usb_manager";
const std::string HASHES_FILE = DIRECTORY_PATH + "/device_hashes.txt";
const std::string WHITELIST_FILE = DIRECTORY_PATH + "/whitelist.txt";
const std::string BLACKLIST_FILE = DIRECTORY_PATH + "/blacklist.txt";
const std::string TOTP_SECRET_FILE = DIRECTORY_PATH + "/totp_secret.txt";
const std::string CONFIG_FILE = DIRECTORY_PATH + "/config.conf";
const std::string LOG_FILE = DIRECTORY_PATH + "/events.log";

const std::string UDEV_RULES_FILE = "/etc/udev/rules.d/90-custom.rules";

// ANSI escape codes
const std::string bluecolor = "\033[1;36m";
const std::string greencolor = "\033[1;32m";
const std::string redcolor = "\033[1;31m";
const std::string resetcolor = "\033[0m";

#endif // CONFIG_H

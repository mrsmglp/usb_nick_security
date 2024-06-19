#include "config.h"
#include "user_management.h"
#include <iostream>
#include <linux/limits.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <cstring>
#include <shadow.h>
#include <crypt.h>
#include <string>
#include <sys/stat.h>
#include <fstream>

std::string getCurrentUserGroup() {
    gid_t gid = getegid();
    struct group *grp = getgrgid(gid);
    return grp ? grp->gr_name : "unknown";
}

std::string getCurrentUser() {
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}
    
bool isPrivilegedUser() {
    gid_t gid = getegid();
    std::ifstream configFile(CONFIG_FILE);
    if (!configFile.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        if (line.find("privileged_gid=") != std::string::npos) {
            gid_t privilegedGid = std::stoi(line.substr(line.find("=") + 1));
            return gid == privilegedGid;
        }
    }
    return false;
}

bool isSuperUser() {
    return geteuid() == 0;
}

void createSpecialUserAndGroup() {
    const std::string username = "specuser1";
    const std::string password = "specuser1";
    const std::string group = "specusers";

    // Создание группы
    std::string command = "groupadd " + group;
    system(command.c_str());

    // Создание пользователя и добавление его в группу
    command = "useradd -m -g " + group + " -G " + group + " -s /bin/bash " + username;
    system(command.c_str());

    // Установка пароля
    command = "echo \"" + username + ":" + password + "\" | chpasswd";
    system(command.c_str());

    // Получение GID группы
    struct group *grp = getgrnam(group.c_str());
    if (!grp) {
        std::cerr << "\nFailed to get GID for group " << group << std::endl;
        return;
    }
    gid_t gid = grp->gr_gid;

    // Настройка sudoers для группы с использованием GID
    std::ofstream sudoersFile("/etc/sudoers.d/specusers");
    if (sudoersFile.is_open()) {
        sudoersFile << "%#" << gid << " ALL=(ALL) NOPASSWD: /usr/bin/nano, /usr/bin/udevadm, /usr/bin/systemctl, /bin/chown, /bin/chmod, /bin/system, /bin/mkdir, /bin/touch\n";
        sudoersFile.close();
    } else {
        std::cerr << "\nFailed to update sudoers file\n" << std::endl;
    }
}

bool checkAndCreateFilesAndDirectories(const std::string& directoryPath, const std::string& hashesFile, const std::string& whitelist, const std::string& blacklist, const std::string& udevRulesFile, const std::string& group) {
    struct stat info;

    // Проверка и создание директории
    if (stat(directoryPath.c_str(), &info) != 0) {
        std::string command = "sudo mkdir -p " + directoryPath;
        system(command.c_str());
        command = "sudo chown root:" + group + " " + directoryPath;
        system(command.c_str());
        command = "sudo chmod 770 " + directoryPath;
        system(command.c_str());
    }

    // Проверка и создание файла device_hashes.txt
    std::ifstream hashesFileStream(hashesFile);
    if (!hashesFileStream.good()) {
        std::string command = "sudo touch " + hashesFile;
        system(command.c_str());
        command = "sudo chown root:" + group + " " + hashesFile;
        system(command.c_str());
        command = "sudo chmod 660 " + hashesFile;
        system(command.c_str());
    }

    // Проверка и создание файла whitelist.txt
    std::ifstream whitelistFileStream(whitelist);
    if (!whitelistFileStream.good()) {
        std::string command = "sudo touch " + whitelist;
        system(command.c_str());
        command = "sudo chown root:" + group + " " + whitelist;
        system(command.c_str());
        command = "sudo chmod 660 " + whitelist;
        system(command.c_str());
    }

    // Проверка и создание файла blacklist.txt
    std::ifstream blacklistFileStream(blacklist);
    if (!blacklistFileStream.good()) {
        std::string command = "sudo touch " + blacklist;
        system(command.c_str());
        command = "sudo chown root:" + group + " " + blacklist;
        system(command.c_str());
        command = "sudo chmod 660 " + blacklist;
        system(command.c_str());
    }

    // Проверка и создание файла 90-custom.rules
     std::ifstream udevFileStream(udevRulesFile);
    if (!udevFileStream.good()) {
        std::string command = "sudo touch " + udevRulesFile;
        system(command.c_str());
        command = "sudo chown root:" + group + " " + udevRulesFile;
        system(command.c_str());
        command = "sudo chmod 660 " + udevRulesFile;
        system(command.c_str());
        
        // Добавление LABEL="end" в файл
        std::ofstream udevFile(udevRulesFile, std::ios_base::app);
        if (udevFile.is_open()) {
            udevFile << "LABEL=\"end\"" << std::endl;
            udevFile.close();
        } else {
            std::cerr << "\nFailed to open udev rules file for writing" << std::endl;
        }
    }

    // Проверка и создание файла events.log
    std::ifstream logFileStream(LOG_FILE);
    if (!logFileStream.good()) {
        std::string command = "sudo touch " + LOG_FILE;
        system(command.c_str());
        command = "sudo chown root:" + group + " " + LOG_FILE;
        system(command.c_str());
        command = "sudo chmod 660 " + LOG_FILE;
        system(command.c_str());
    }

    // Проверка и создание файла config.txt
    std::ifstream configFileStream(CONFIG_FILE);
    if (!configFileStream.good()) {
        std::ofstream configFile(CONFIG_FILE);
        if (configFile.is_open()) {
            // Получение GID группы
            struct group *grp = getgrnam(group.c_str());
            if (grp) {
                gid_t gid = grp->gr_gid;
                configFile << "privileged_gid=" << gid << std::endl;
            } else {
                std::cerr << "\nFailed to get GID for group " << group << std::endl;
            }
            configFile.close();
        } else {
            std::cerr << "\nFailed to create config file." << std::endl;
        }
        std::string command = "sudo chown root:" + group + " " + CONFIG_FILE;
        system(command.c_str());
        command = "sudo chmod 660 " + CONFIG_FILE;
        system(command.c_str());
    }

    return true;
}

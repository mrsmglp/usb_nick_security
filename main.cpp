#include "config.h"
#include "logger.h"
#include "usb_monitor.h"
#include "device_manager.h"
#include "user_management.h"
#include "totp.h"
#include <unistd.h>
#include <limits.h>
#include <iostream>
#include <limits>
#include <string>
#include <fstream>
#include <thread>
#include <sys/stat.h>

// Объект класса DeviceManager
DeviceManager deviceManager;

void printAsciiArt() {
    std::cout << "\n\n\n";
    std::cout << "██╗░░░██╗░██████╗██████╗░  ███╗░░██╗██╗░█████╗░██╗░░██╗\n";
    std::cout << "██║░░░██║██╔════╝██╔══██╗  ████╗░██║██║██╔══██╗██║░██╔╝\n";
    std::cout << "██║░░░██║╚█████╗░██████╦╝  ██╔██╗██║██║██║░░╚═╝█████═╝░\n";
    std::cout << "██║░░░██║░╚═══██╗██╔══██╗  ██║╚████║██║██║░░██╗██╔═██╗░\n";
    std::cout << "╚██████╔╝██████╔╝██████╦╝  ██║░╚███║██║╚█████╔╝██║░╚██╗\n";
    std::cout << "░╚═════╝░╚═════╝░╚═════╝░  ╚═╝░░╚══╝╚═╝░╚════╝░╚═╝░░╚═╝\n";
    std::cout << "\n";
    std::cout << "░██████╗███████╗░█████╗░██╗░░░██╗██████╗░██╗████████╗██╗░░░██╗\n";
    std::cout << "██╔════╝██╔════╝██╔══██╗██║░░░██║██╔══██╗██║╚══██╔══╝╚██╗░██╔╝\n";
    std::cout << "╚█████╗░█████╗░░██║░░╚═╝██║░░░██║██████╔╝██║░░░██║░░░░╚████╔╝░\n";
    std::cout << "░╚═══██╗██╔══╝░░██║░░██╗██║░░░██║██╔══██╗██║░░░██║░░░░░╚██╔╝░░\n";
    std::cout << "██████╔╝███████╗╚█████╔╝╚██████╔╝██║░░██║██║░░░██║░░░░░░██║░░░\n";
    std::cout << "╚═════╝░╚══════╝░╚════╝░░╚═════╝░╚═╝░░╚═╝╚═╝░░░╚═╝░░░░░░╚═╝░░░\n";
     std::cout << "\n\n";
}

void mainMenu() {
    std::string currentUser = getCurrentUser();
    std::string currentGroup = getCurrentUserGroup();
    std::cout << greencolor << "\nEnter a command ('show all', 'add rules', 'edit rules', 'edit lists', 'show logs', 'exit')" << resetcolor << "\n";
    std::cout << bluecolor << "\n" << currentUser << " & " << currentGroup << ": " << resetcolor;
}

void processUserCommand(const std::string& input) {
    if (input == "show all") {
        logEvent({getCurrentUser(), "selected show all"});
        showAllDevices(deviceManager);
        mainMenu();
    } else if (input == "add rules") {
        logEvent({getCurrentUser(), "selected add rules"});
        if (!isPrivilegedUser()) {
            std::cerr << "\nAccess denied. Only privileged users can add rules." << std::endl;
            mainMenu();
            return;
        }

        std::cout << "\n1) Add Allow Rule" << std::endl;
        std::cout << "2) Add Block Rule" << std::endl;
        std::cout << "3) Add Reject Rule" << std::endl;
        std::cout << "Choose an action (1-3, or 0 to cancel): ";
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\nInvalid input. Please enter a number (1-3, or 0 to cancel)." << std::endl;
            mainMenu();
            return;
        }
        std::cin.ignore();
        switch (choice) {
            case 1:
                addAllowRule();
                logEvent({getCurrentUser(), "choose Add Allow Rule"});
                break;
            case 2:
                addBlockRule();
                logEvent({getCurrentUser(), "choose Add Block Rule"});
                break;
            case 3:
                addRejectRule();
                logEvent({getCurrentUser(), "choose Add Reject Rule"});
                break;
            case 0:
                std::cout << "\nCanceled...." << std::endl;
                logEvent({getCurrentUser(), "canceled add rules"});
                break;
            default:
                std::cout << "\nInvalid choice!" << std::endl;
                break;
        }
        mainMenu();
    } else if (input == "edit rules") {
        logEvent({getCurrentUser(), "selected edit rules"});
        if (!isPrivilegedUser()) {
            std::cerr << "\nAccess denied. Only privileged users can edit rules." << std::endl;
            mainMenu();
            return;
        }

        std::cout << "\nOpening text editor...." << std::endl;
        openFileInEditor(UDEV_RULES_FILE);
        mainMenu();
    } else if (input == "edit lists") {
        logEvent({getCurrentUser(), "selected edit lists"});
        if (!isPrivilegedUser()) {
            std::cerr << "\nAccess denied. Only privileged users can edit lists." << std::endl;
            mainMenu();
            return;
        }

        std::cout << "\n1) Edit Whitelist" << std::endl;
        std::cout << "2) Edit Blacklist" << std::endl;
        std::cout << "Choose a list to edit (1-2, or 0 to cancel): ";
        int listChoice;
        if (!(std::cin >> listChoice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\nInvalid input. Please enter a number (1-2, or 0 to cancel)." << std::endl;
            mainMenu();
            return;
        }
        std::cin.ignore();

        if (listChoice == 0) {
            std::cout << "\nCanceled...." << std::endl;
            logEvent({getCurrentUser(), "canceled edit lists"});
            mainMenu();
            return;
        }

        std::cout << "\n1) Add Hash" << std::endl;
        std::cout << "2) Remove Hash" << std::endl;
        std::cout << "Choose an action (1-2, or 0 to cancel): ";
        int actionChoice;
        if (!(std::cin >> actionChoice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\nInvalid input. Please enter a number (1-2, or 0 to cancel)." << std::endl;
            mainMenu();
            return;
        }
        std::cin.ignore();

        if (actionChoice == 0) {
            std::cout << "\nCanceled...." << std::endl;
            logEvent({getCurrentUser(), "canceled edit lists action"});
            mainMenu();
            return;
        }

        std::string deviceHash;
        std::cout << "\nEnter device hash: ";
        std::getline(std::cin, deviceHash);

        if (listChoice == 1) {
            if (actionChoice == 1) {
                deviceManager.addToWhitelist(deviceHash);
                deviceManager.saveWhitelistToFile(WHITELIST_FILE);
                std::cout << "\nDevice added to whitelist." << std::endl;
                logEvent({getCurrentUser(), "added device to whitelist"});
            } else if (actionChoice == 2) {
                deviceManager.removeFromWhitelist(deviceHash);
                deviceManager.saveWhitelistToFile(WHITELIST_FILE);
                std::cout << "\nDevice removed from whitelist." << std::endl;
                logEvent({getCurrentUser(), "removed device from whitelist"});
            }
        } else if (listChoice == 2) {
            if (actionChoice == 1) {
                deviceManager.addToBlacklist(deviceHash);
                deviceManager.saveBlacklistToFile(BLACKLIST_FILE);
                std::cout << "\nDevice added to blacklist." << std::endl;
                logEvent({getCurrentUser(), "added device to blacklist"});
            } else if (actionChoice == 2) {
                deviceManager.removeFromBlacklist(deviceHash);
                deviceManager.saveBlacklistToFile(BLACKLIST_FILE);
                std::cout << "\nDevice removed from blacklist." << std::endl;
                logEvent({getCurrentUser(), "removed device from blacklist"});
            }
        }
        mainMenu();
    } else if (input == "show logs") {
        std::ifstream logFile(LOG_FILE);
            if (!logFile.is_open()) {
            std::cerr << "\nFailed to open log file: " << LOG_FILE << std::endl;
            return;
        }

        // Проверка на пустой файл
        logFile.seekg(0, std::ios::end);
            if (logFile.tellg() == 0) {
            std::cout << "\nLog file is empty." << std::endl;
            logFile.close();
            return;
            }
        logFile.seekg(0, std::ios::beg);

        std::string line;
        while (std::getline(logFile, line)) {
        if (line.size() >= 19) {
            // Предполагаем, что дата занимает первые 19 символов в каждой строке лога
            std::string datePart = line.substr(0, 19);
            std::string eventPart = line.substr(19);
            std::cout << bluecolor << datePart << resetcolor << eventPart << std::endl;
        } else {
            // Если строка короче 19 символов, выводим ее без изменений
            std::cout << line << std::endl;
        }
    }

    logFile.close();
    mainMenu();
    } else if (input == "exit") {
        logEvent({getCurrentUser(), "exited the program"});
        clearScreen();
        return;
    } else {
        std::cout << "\nInvalid command!" << std::endl;
        mainMenu();
    }
}

int main() {
    std::string programPath = getProgramPath();
    std::string programDir = programPath.substr(0, programPath.find_last_of('/'));

    logEvent({getCurrentUser(), "started the program"});

    // Проверка начальной настройки
    struct stat buffer;
    bool initialSetupRequired = (stat(DIRECTORY_PATH.c_str(), &buffer) != 0) ||
                                (stat(HASHES_FILE.c_str(), &buffer) != 0) ||
                                (stat(UDEV_RULES_FILE.c_str(), &buffer) != 0);

    if (initialSetupRequired) {
        if (!isSuperUser()) {
        std::cerr << redcolor << "\nThis program must be run as root for initial setup." << resetcolor << std::endl;
        return 1;
        }

    createSpecialUserAndGroup();
    checkAndCreateFilesAndDirectories(DIRECTORY_PATH, HASHES_FILE, WHITELIST_FILE, BLACKLIST_FILE, UDEV_RULES_FILE, "specusers");

    // Копирование исполняемого файла в домашнюю директорию пользователя
    std::string command = "cp " + programPath + " /home/specuser1/usb_nicksec";
    system(command.c_str());
    command = "chown specuser1:specusers /home/specuser1/usb_nicksec";
    system(command.c_str());
    command = "chmod 750 /home/specuser1/usb_nicksec";
    system(command.c_str());

    // Генерация секретного ключа и QR-кода
    std::string secret = generateTOTPSecret();
    std::ofstream secretFile(DIRECTORY_PATH + "/totp_secret.txt");
    if (secretFile) {
        secretFile << secret;
        secretFile.close();
        
        // Установка владельца и прав доступа для файла totp_secret.txt
        command = "chown root:specusers " + std::string(DIRECTORY_PATH + "/totp_secret.txt");
        system(command.c_str());
        command = "chmod 660 " + std::string(DIRECTORY_PATH + "/totp_secret.txt");
        system(command.c_str());
        
        std::cout << "\nPlease scan QR Code by using Google Authenticator or another authenticator app\n" << std::endl;
        generateQRCode(secret, "specuser1");
        std::cout << greencolor << "\nInitial setup completed." << resetcolor << std::endl;
        
        std::cout << "\ndefault user: specuser1" << std::endl;
        std::cout << "default password: specuser1" << std::endl;
        std::cout << "default group: specusers" << std::endl;
    } else {
        std::cerr << redcolor << "\nFailed to create TOTP secret file." << resetcolor << std::endl;
    }

    return 0;
    }

    if (isSuperUser()) {
        std::cerr << redcolor << "\nThis program should be run as the special user, not as root." << resetcolor << std::endl;
        return 1;
    }

    if (!isPrivilegedUser()) {
        std::cerr << redcolor << "\nThis program should be run as the special user." << resetcolor << std::endl;
        return 1;
    }

    // Загрузка хэшей и черных-белых списков из файлов при запуске программы
    deviceManager.loadHashesFromFile(HASHES_FILE);
    deviceManager.loadWhitelistFromFile(WHITELIST_FILE);
    deviceManager.loadBlacklistFromFile(BLACKLIST_FILE);
    
    // Запуск мониторинга устройств в отдельном потоке
    std::thread monitorThread(monitorDevices, std::ref(deviceManager));

    // TOTP аутентификация
    std::ifstream secretFile(DIRECTORY_PATH + "/totp_secret.txt");
    if (!secretFile) {
        std::cerr << redcolor << "\nFailed to open TOTP secret file." << resetcolor << std::endl;
        return 1;
    }

    std::string secret;
    std::getline(secretFile, secret);
    secretFile.close();

    // Запрос у пользователя, хочет ли он просканировать QR-код
    std::string rescanChoice;
    std::cout << "\nDo you want to rescan the QR code? (yes/no): ";
    std::cin >> rescanChoice;
    if (rescanChoice == "yes") {
        generateQRCode(secret, "specuser1");
    }

    int attempts = 0;
    const int maxAttempts = 3;
    std::string userInput;

     while (attempts < maxAttempts) {
        std::cout << "\nEnter TOTP code: ";
        std::cin >> userInput;

        std::string expectedCode = generateTOTPCode(secret);
        if (userInput == expectedCode) {
            std::cout << greencolor << "\nTOTP authentication successful." << resetcolor << std::endl;
            logEvent({getCurrentUser(), "TOTP authentication successful on attempt", std::to_string(attempts + 1)});
            break;
        } else {
            std::cerr << redcolor << "\nInvalid TOTP code." << resetcolor << std::endl;
            attempts++;
        }

        if (attempts >= maxAttempts) {
            std::cerr << redcolor << "\nMaximum attempts exceeded. Exiting..." << resetcolor << std::endl;
            logEvent({getCurrentUser(), "TOTP authentication failed after", std::to_string(maxAttempts), "attempts"});
            return 1;
        }
    }

    std::cout << greencolor << "\nTOTP authentication successful." << resetcolor << std::endl;

    // Очищаем консоль перед запуском программы
    clearScreen();

    // Печать приветственного сообщения
    printAsciiArt();

    // Печать главного меню
    mainMenu();

    // Основное диалоговое меню пользователя
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        processUserCommand(input);
        if (input == "exit") {
            deviceManager.setStopMonitoring(true);
            monitorThread.join();
            return 0;
        }
    }

    // Сохранение хэшей в файл при завершении программы
    deviceManager.saveHashesToFile(HASHES_FILE);

    return 0;
}

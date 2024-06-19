#ifndef USER_MANAGEMENT_H
#define USER_MANAGEMENT_H

#include <string>

// Получение текущей группы пользователя и логина
std::string getCurrentUserGroup();
std::string getCurrentUser();

// Проверка, явялется ли пользователь особым
bool isPrivilegedUser();

// Проверка, является ли текущий пользователь суперпользователем
bool isSuperUser();

// Создание специального пользователя и группы
void createSpecialUserAndGroup();

// Проверка и создание файлов и директорий
bool checkAndCreateFilesAndDirectories(const std::string& directoryPath, const std::string& hashesFile, const std::string& whitelist, const std::string& blacklist, const std::string& udevRulesFile, const std::string& group);

#endif // USER_MANAGEMENT_H

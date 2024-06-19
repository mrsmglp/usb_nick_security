#ifndef TOTP_H
#define TOTP_H

#include <string>

// Функция генерациия TOTP секрета
std::string generateTOTPSecret();

// Функция генерации QR кода
void generateQRCode(const std::string& secret, const std::string& username);

// Функция генерации TOTP кода
std::string generateTOTPCode(const std::string& secret);

// Функция декодирования
std::string base32Decode(const std::string& encoded);

#endif // TOTP_H

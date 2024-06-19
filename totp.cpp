#include "totp.h"
#include <qrencode.h>
#include <openssl/evp.h>
#include <ctime>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>

std::string generateTOTPSecret() {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string secret;
    for (int i = 0; i < 16; ++i) {
        secret += chars[rand() % 32];
    }
    return secret;
}

void generateQRCode(const std::string& secret, const std::string& username) {
    std::string url = "otpauth://totp/USB Nick Security:" + username + "?secret=" + secret + "&issuer=USB Nick Security";
    QRcode* qr = QRcode_encodeString(url.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1);
    if (qr) {
        for (int y = 0; y < qr->width; ++y) {
            for (int x = 0; x < qr->width; ++x) {
                std::cout << (qr->data[y * qr->width + x] & 1 ? "██" : "  ");
            }
            std::cout << std::endl;
        }
        QRcode_free(qr);
    }
}

std::string base32Decode(const std::string& encoded) {
    static const char* base32Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string binary;
    int buffer = 0, bitsLeft = 0;
    for (char c : encoded) {
        if (c == '=') break;
        const char* p = strchr(base32Chars, c);
        if (p == nullptr) return "";
        buffer <<= 5;
        buffer |= (p - base32Chars);
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            binary += char((buffer >> (bitsLeft - 8)) & 0xFF);
            bitsLeft -= 8;
        }
    }
    return binary;
}

std::string generateTOTPCode(const std::string& secret) {
    std::string decodedSecret = base32Decode(secret);
    if (decodedSecret.empty()) return "";

    std::uint64_t time = std::time(nullptr) / 30;
    unsigned char timeBytes[8];
    for (int i = 7; i >= 0; --i) {
        timeBytes[i] = time & 0xFF;
        time >>= 8;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    size_t hash_len;
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if (!mac) {
        std::cerr << "\nFailed to fetch HMAC" << std::endl;
        return "";
    }

    EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
    if (!ctx) {
        EVP_MAC_free(mac);
        std::cerr << "\nFailed to create HMAC context" << std::endl;
        return "";
    }

    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string("digest", const_cast<char *>("SHA1"), 0),
        OSSL_PARAM_construct_end()
    };

    if (EVP_MAC_init(ctx, reinterpret_cast<const unsigned char*>(decodedSecret.data()), decodedSecret.size(), params) != 1 ||
        EVP_MAC_update(ctx, timeBytes, sizeof(timeBytes)) != 1 ||
        EVP_MAC_final(ctx, hash, &hash_len, sizeof(hash)) != 1) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        std::cerr << "\nFailed to compute HMAC" << std::endl;
        return "";
    }

    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);

    int offset = hash[hash_len - 1] & 0xF;
    int binary = ((hash[offset] & 0x7F) << 24) |
                 ((hash[offset + 1] & 0xFF) << 16) |
                 ((hash[offset + 2] & 0xFF) << 8) |
                 (hash[offset + 3] & 0xFF);

    int otp = binary % 1000000;
    std::ostringstream result;
    result << std::setw(6) << std::setfill('0') << otp;
    return result.str();
}

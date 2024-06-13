#include "dongle_api.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#define MAX_OUTPUT_LEN 128

#ifndef LICENSE_TAG
#define LICENSE_TAG "TAG"
#endif
#ifndef LICENSE_ADMIN
#define LICENSE_ADMIN "ADMIN"
#endif
#ifndef RANDOMS
#define RANDOMS "123456"
#endif

namespace glm {
// Remove trailing spaces from a string
__attribute__((always_inline)) inline bool isSpaceOrNewline(char c) {
    return c == ' ' || c == '\n';
}

__attribute__((always_inline)) inline std::string rtrimSpaceAndNewline(std::string s) {
    while (!s.empty() && isSpaceOrNewline(s.back())) {
        s.pop_back(); // remove characters from the end
    }
    return s;
}

// get command output
__attribute__((always_inline)) inline bool fileExists(const std::string &filename) {
    std::ifstream file(filename);
    return file.good();
}

__attribute__((always_inline)) inline std::string commandOutput(const std::string &cmd) {
    std::array<char, MAX_OUTPUT_LEN> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    try {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += rtrimSpaceAndNewline(std::string(buffer.data()));
        }
    } catch (...) {
        throw;
    }

    return result;
}

// get hardware information
__attribute__((always_inline)) inline std::array<unsigned char, 512> getEnv() {
#define INOUT_BUF_LENGTH 1024
    // int fileid = 0x0002;
    // int ret = 0;

    // fetch hardware information example
    std::string board_name_path = "/sys/class/dmi/id/board_name";
    std::string board_name;
    if (fileExists(board_name_path)) {
        board_name = commandOutput("cat " + board_name_path);
    }

    std::string board_serial_path = "/sys/class/dmi/id/board_serial";
    std::string board_serial;
    if (fileExists(board_serial_path)) {
        board_serial = commandOutput("cat " + board_serial_path);
    }

    std::string board_vendor_path = "/sys/class/dmi/id/board_vendor";
    std::string board_vendor;
    if (fileExists(board_vendor_path)) {
        board_vendor = commandOutput("cat " + board_vendor_path);
    }

    std::string product_name_path = "/sys/class/dmi/id/product_name";
    std::string product_name;
    if (fileExists(product_name_path)) {
        product_name = commandOutput("cat " + product_name_path);
    }

    std::string product_version_path = "/sys/class/dmi/id/product_version";
    std::string product_version;
    if (fileExists(product_version_path)) {
        product_version = commandOutput("cat " + product_version_path);
    }

    std::string product_serial_path = "/sys/class/dmi/id/product_serial";
    std::string product_serial;
    if (fileExists(product_serial_path)) {
        product_serial = commandOutput("cat " + product_serial_path);
    }

    std::string product_uuid_path = "/sys/class/dmi/id/product_uuid";
    std::string product_uuid;
    if (fileExists(product_uuid_path)) {
        product_uuid = commandOutput("cat " + product_uuid_path);
    }

    std::array<char, INOUT_BUF_LENGTH> InOutBuf;
    std::snprintf(InOutBuf.data(), InOutBuf.size(),
                  "board_name:%s,board_serial:%s,board_vendor:%s,product_name:%s,product_version:%s,"
                  "product_serial:%s,product_uuid:%s",
                  board_name.c_str(), board_serial.c_str(), board_vendor.c_str(), product_name.c_str(),
                  product_version.c_str(), product_serial.c_str(), product_uuid.c_str());
    // Inoutbuf转换为数组
    std::array<unsigned char, 512> info;
    for (std ::size_t i = 0; i < info.size(); i++) {
        info[i] = static_cast<unsigned char>(InOutBuf[i]);
    }
    return info;
}

__attribute__((always_inline)) inline std::string base64Decode(const std::string &data) {
    BIO *bio, *b64;

    int decodeLength = data.size();
    std::vector<char> buffer(decodeLength);

    bio = BIO_new_mem_buf(data.c_str(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer
    decodeLength = BIO_read(bio, buffer.data(), data.size());

    BIO_free_all(bio);

    return std::string(buffer.data(), decodeLength);
}

__attribute__((always_inline)) inline RSA *loadPublicKey() {
    std::string publicKeyPem = "-----BEGIN PUBLIC KEY-----\n"
                               "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1HwENS507VpuXLfan7wJ\n"
                               "tOE5YtpvzuaBr6W+QrH3eLlA+zpds4PENqJkya75ESs0SJqJPqZWufTphibZ+l8y\n"
                               "LpMjOZdtQDoWQ+fW0/CVpGgTBbPF0moL+9FxwOJK8N6aAJrcGlpKV6968JOAS0n6\n"
                               "XxEoThnywy2EF/GhEPAS55Gm/OmIM/GzqrtPkVVr2sUNV79dwxFBHAynDWfxHlJk\n"
                               "6ONKPV9wETVITSNF6E1VISWu/kDyvNcRp5h32BQzE6QDocUssbF6X+DcM0wp5Isz\n"
                               "1iV5m/ngF/C8m9fb7L6BStfYghSB4eOdUhAilReR69datl5NWDqj+ZRrK1m5fnXh\n"
                               "gwIDAQAB\n"
                               "-----END PUBLIC KEY-----\n";
    BIO *bio = BIO_new_mem_buf(publicKeyPem.c_str(), -1);
    if (bio == nullptr) {
        throw std::runtime_error("fail to create bio for publicKey");
    }

    RSA *rsa_public_key = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
    if (rsa_public_key == nullptr) {
        BIO_free_all(bio);
        throw std::runtime_error("fail to load publicKey");
    }

    BIO_free_all(bio);
    return rsa_public_key;
}

__attribute__((always_inline)) inline bool
rsaVerify(const std::string &data, const std::vector<unsigned char> &signature, RSA *public_key) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (!SHA256((unsigned char *)data.c_str(), data.size(), hash)) {
        throw std::runtime_error("fail to compute SHA-256 hash");
    }

    // Verify the signature
    if (RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, signature.data(), signature.size(), public_key) != 1) {
        char *err = ERR_error_string(ERR_get_error(), NULL);
        std::cout << "OpenSSL Error: " << err << std::endl;
        return false;
    }
    return true;
}

__attribute__((always_inline)) inline std::vector<unsigned char> hexToBytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

__attribute__((always_inline)) inline std::string aesDecrypt(const std::vector<unsigned char> &ciphertext,
                                                             const std::vector<unsigned char> &key,
                                                             const std::vector<unsigned char> &iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    if (!ctx) {
        throw std::runtime_error("fail to create EVP_CIPHER_CTX");
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data()) != 1) {
        throw std::runtime_error("fail to initialize decryption");
    }

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        throw std::runtime_error("fail to decrypt data");
    }

    int plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        char *err = ERR_error_string(ERR_get_error(), NULL);
        // std::cout << "OpenSSL Decrypt Error: " << err << std::endl;
        throw std::runtime_error(err);
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);

    return std::string((char *)plaintext.data(), plaintext_len);
}

__attribute__((always_inline)) inline void check_license(const std::filesystem::path &path) {
    std::ifstream license_file(std::filesystem::path(path) / "license.txt");
    if (!license_file.is_open()) {
        throw std::runtime_error("license file not exist!");
    }
    std::ostringstream license_stream;
    license_stream << license_file.rdbuf();
    std::string license = license_stream.str();
    std::string license_decode = base64Decode(license);

    // aes decrypt
    std::string key_hex = "d37dffe9718aa504f3624518628b3156b88e8ac8552b4a892ac02cfd93b40c16";
    std::string iv_hex = "20b2af9f545582950f267dec8596d190";
    std::vector<unsigned char> key = hexToBytes(key_hex);
    std::vector<unsigned char> iv = hexToBytes(iv_hex);
    std::vector<unsigned char> ciphertext = hexToBytes(license_decode);
    std::string license_data = aesDecrypt(ciphertext, key, iv);
    // std::cout << "license_data: " << license_data << std::endl;
    license_decode = license_data;

    std::string data = license_decode.substr(0, license_decode.find_last_of('|'));
    std::istringstream ss(license_decode);
    std::string license_tag, license_env, license_endtime, license_signature;
    std::getline(ss, license_tag, '|');
    std::getline(ss, license_env, '|');
    std::getline(ss, license_endtime, '|');
    std::getline(ss, license_signature, '|');

    // check license signature
    RSA *public_key = loadPublicKey();
    std::vector<unsigned char> signature = hexToBytes(license_signature);
    // std::cout << "Signature length: "<< signature.size() << std::endl;
    if (!rsaVerify(data, signature, public_key)) {
        throw std::runtime_error("fail to verify signature");
    }

    // check license_endtime
    char *license_admin_pwd = std::getenv("LICENSE_ADMIN_PWD");
    time_t now = 0;
    // std::cout << "LICENSE_ADMIN_PWD: " << (license_admin_pwd ? license_admin_pwd : "NULL") << std::endl;
    // std::cout << "LICENSE_ADMIN: " << LICENSE_ADMIN << std::endl;
    if (license_admin_pwd && std::string(license_admin_pwd) == (LICENSE_ADMIN)) {
        now = time(0);
    } else {
        DWORD dwRet = 0;
        int nCount = 0;
        int i = 0;
        int nIndex = -1;
        DWORD dwTime = 0;
        char UserPin[] = "aminer123";
        int nRemainCount = 0;
        DONGLE_INFO *pDongleInfo = NULL;
        DONGLE_HANDLE hDongle = NULL;
        // 枚举锁
        // 读取文件，加文件锁，防止多个进程同时操作
        // 判断path下dongle.json是否存在，如果不存在，创建
        // std::string dongle_json = path / "dongle.json";
        // std::ifstream dongle_json_file(dongle_json);
        // if (!dongle_json_file.is_open()) {
        //     // create dongle.json
        //     std::ofstream dongle_json_file(dongle_json);
        //     dongle_json_file << "{\"dongle\": \"0\"}";
        // } else {
        //     // read dongle.json
        //     std::ostringstream dongle_json_stream;
        //     dongle_json_stream << dongle_json_file.rdbuf();
        //     std::string dongle_json_str = dongle_json_stream.str();
        //     std::cout << "dongle_json_str: " << dongle_json_str << std::endl;
        //     // parse dongle.json
        //     std::istringstream dongle_json_ss(dongle_json_str);
        //     std::string dongle_str;
        //     std::getline(dongle_json_ss, dongle_str, ':');
        //     std::getline(dongle_json_ss, dongle_str, '}');
        //     // std::cout << "dongle_str: " << dongle_str << std::endl;
        //     for (int i = 0; i < 100; i++) {
        //         if (dongle_str == "0") {
        //             break;
        //         }
        //         sleep(1);

        //     }
        // }
        for (int i = 0; i < 1000; i++) {
            dwRet = Dongle_Enum(NULL, &nCount);
            if (dwRet != 0 || nCount == 0) {
                usleep(10000);
                continue;
            }
            break;
        }

        // dwRet = Dongle_Enum(NULL, &nCount);
        // if (dwRet != 0 || nCount == 0) {
        //     throw std::runtime_error("Can't Enum Dongle, Please Insert Usb Key");
        // }
        printf("Enum %d Dongle ARM. \n", nCount);
        pDongleInfo = (DONGLE_INFO *)malloc(nCount * sizeof(DONGLE_INFO));
        dwRet = Dongle_Enum(pDongleInfo, &nCount);
        for (i = 0; i < nCount; i++) {
            if (pDongleInfo[i].m_Type == 0 || pDongleInfo[i].m_Type == 1) {
                nIndex = i;
            }
        }
        if (nIndex == -1) { // 没有找到时钟锁
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("Can't Find Dongle, Please Insert Usb Key");
            // Dongle_Close(hDongle);
        }
        // 打开锁
        dwRet = Dongle_Open(&hDongle, 0);
        if (DONGLE_SUCCESS != dwRet) {
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("Can't Open Dongle, Please Insert Usb Key");
        }
        printf("Open Dongle ARM. Return : 0x%08X . \n", dwRet);
        // 验证锁用户密码
        dwRet = Dongle_VerifyPIN(hDongle, FLAG_USERPIN, UserPin, &nRemainCount);
        if (DONGLE_SUCCESS != dwRet) {
            Dongle_Close(hDongle);
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("Please Insert Correct Usb Key.");
        }

        // 获取锁内时间
        dwRet = Dongle_GetUTCTime(hDongle, &dwTime);
        // printf("Get UTC Time. Return: 0x%08X\n", dwRet);
        // printf("Current Time. ReturnL 0x%08X\n", dwTime);
        if (DONGLE_SUCCESS == dwRet) {
            time_t actualTime = dwTime;
            tm *utm = gmtime(&actualTime);
            now = mktime(utm);
        } else {
            Dongle_Close(hDongle);
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("Get Dongle Time Failed, Please Contact ZHIPU Replace USB Key.");
        }
        // 关闭加密锁
        dwRet = Dongle_Close(hDongle);
        printf("Close Dongle ARM. Return: 0x%08X\n", dwRet);
        if (pDongleInfo != NULL) {
            free(pDongleInfo);
            pDongleInfo = NULL;
        }
    }
    printf("Now time: %ld\n", now);
    printf("License Expire time: %ld\n", std::stol(license_endtime));
    if (now > std::stoi(license_endtime)) {
        throw std::runtime_error("license is expired!");
    }

    // check license_tag
    if (license_tag != LICENSE_TAG) {
        throw std::runtime_error("license tag is not matched!");
    }

    // check license_env
    if (license_env == "anymachine") {
        std::cout << "license is valid!\n";
    } else {
        std::array<unsigned char, 512> licenseEnv = getEnv();
        std::string info = std::string((char *)licenseEnv.data(), licenseEnv.size());
        info = info.substr(0, info.find('\0'));
        // std::cout << "license is: " << license << "\n";
        std::string delimiter = ";";
        size_t pos = 0;
        std::string token;
        bool found = false;
        while ((pos = license_env.find(delimiter)) != std::string::npos) {
            token = license_env.substr(0, pos);
            // std::cout << token << std::endl;
            if (token == info) {
                found = true;
                break;
            }
            license_env.erase(0, pos + delimiter.length());
        }
        if (!found && license_env != info) {
            throw std::runtime_error("license env is invalid!");
        }
        std::cout << "license is valid!\n";
    }
}
__attribute__((always_inline)) inline std::string get_nonce() {
    return std::string(RANDOMS);
}

} // namespace glm

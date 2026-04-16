#include "dongle_api.h"
#include <array>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <pybind11/pybind11.h>
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
#define OPENSSL_NO_DEPRECATED


#ifndef LICENSE_TAG
#define LICENSE_TAG "8pwkk3mq"
#endif
#ifndef LICENSE_ADMIN
#define LICENSE_ADMIN "pYBZa4NU292L39iWjSO6Yyrp4ZQptVeP"
#endif
#ifndef RANDOMS
#define RANDOMS "123456"
#endif

namespace py = pybind11;

namespace glm
{
    // Remove trailing spaces from a string
    __attribute__((always_inline)) inline bool isSpaceOrNewline(char c) { return c == ' ' || c == '\n'; }

    __attribute__((always_inline)) inline std::string rtrimSpaceAndNewline(std::string s)
    {
        while (!s.empty() && isSpaceOrNewline(s.back()))
        {
            s.pop_back(); // remove characters from the end
        }
        return s;
    }

    // get command output
    __attribute__((always_inline)) inline bool fileExists(const std::string &filename)
    {
        std::ifstream file(filename);
        return file.good();
    }

    __attribute__((always_inline)) inline std::string commandOutput(const std::string &cmd)
    {
        std::array<char, MAX_OUTPUT_LEN> buffer;
        std::string result;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("License Error : Open License Failed!");
        }
        try
        {
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
            {
                result += rtrimSpaceAndNewline(std::string(buffer.data()));
            }
        }
        catch (...)
        {
            throw;
        }

        return result;
    }

    // get hardware information
    __attribute__((always_inline)) inline std::array<unsigned char, 512> getEnv()
    {

#define INOUT_BUF_LENGTH 1024
        // int fileid = 0x0002;
        // int ret = 0;

        // fetch hardware information example
        std::string board_name_path = "/sys/class/dmi/id/board_name";
        std::string board_name;
        if (fileExists(board_name_path))
        {
            board_name = commandOutput("cat " + board_name_path);
        }

        std::string board_serial_path = "/sys/class/dmi/id/board_serial";
        std::string board_serial;
        if (fileExists(board_serial_path))
        {
            board_serial = commandOutput("cat " + board_serial_path);
        }

        std::string board_vendor_path = "/sys/class/dmi/id/board_vendor";
        std::string board_vendor;
        if (fileExists(board_vendor_path))
        {
            board_vendor = commandOutput("cat " + board_vendor_path);
        }

        std::string product_name_path = "/sys/class/dmi/id/product_name";
        std::string product_name;
        if (fileExists(product_name_path))
        {
            product_name = commandOutput("cat " + product_name_path);
        }

        std::string product_version_path = "/sys/class/dmi/id/product_version";
        std::string product_version;
        if (fileExists(product_version_path))
        {
            product_version = commandOutput("cat " + product_version_path);
        }

        std::string product_serial_path = "/sys/class/dmi/id/product_serial";
        std::string product_serial;
        if (fileExists(product_serial_path))
        {
            product_serial = commandOutput("cat " + product_serial_path);
        }

        std::string product_uuid_path = "/sys/class/dmi/id/product_uuid";
        std::string product_uuid;
        if (fileExists(product_uuid_path))
        {
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
        for (std ::size_t i = 0; i < info.size(); i++)
        {
            info[i] = static_cast<unsigned char>(InOutBuf[i]);
        }
        return info;
    }

    __attribute__((always_inline)) inline std::string base64Decode(const std::string &data)
    {
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

    __attribute__((always_inline)) inline RSA *loadPublicKey()
    {
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
        if (bio == nullptr)
        {
            throw std::runtime_error("License Error : Failed To Create Bio For PublicKey");
        }

        RSA *rsa_public_key = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
        if (rsa_public_key == nullptr)
        {
            BIO_free_all(bio);
            throw std::runtime_error("License Error : Failed To Load PublicKey");
        }

        BIO_free_all(bio);
        return rsa_public_key;
    }

    __attribute__((always_inline)) inline bool
    rsaVerify(const std::string &data, const std::vector<unsigned char> &signature, RSA *public_key)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        if (!SHA256((unsigned char *)data.c_str(), data.size(), hash))
        {
            throw std::runtime_error("License Error : Failed to compute SHA-256 Hash");
        }

        // Verify the signature
        if (RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, signature.data(), signature.size(), public_key) != 1)
        {
            char *err = ERR_error_string(ERR_get_error(), NULL);
            std::cout << "OpenSSL Error: " << err << std::endl;
            return false;
        }
        return true;
    }

    __attribute__((always_inline)) inline std::vector<unsigned char> hexToBytes(const std::string &hex)
    {
        std::vector<unsigned char> bytes;
        for (unsigned int i = 0; i < hex.length(); i += 2)
        {
            std::string byteString = hex.substr(i, 2);
            unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
            bytes.push_back(byte);
        }

        return bytes;
    }

    __attribute__((always_inline)) inline std::string aesDecrypt(const std::vector<unsigned char> &ciphertext,
                                                                 const std::vector<unsigned char> &key,
                                                                 const std::vector<unsigned char> &iv)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

        if (!ctx)
        {
            throw std::runtime_error("License Error : Failed To Create EVP_CIPHER_CTX");
        }

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data()) != 1)
        {
            throw std::runtime_error("License Error : Failed To Initialize Decryption");
        }

        std::vector<unsigned char> plaintext(ciphertext.size());
        int len;

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1)
        {
            throw std::runtime_error("License Error : Failed To Decrypt Data");
        }

        int plaintext_len = len;

        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1)
        {
            char *err = ERR_error_string(ERR_get_error(), NULL);
            // std::cout << "OpenSSL Decrypt Error: " << err << std::endl;
            throw std::runtime_error("License Error : " + std::string(err));
        }

        plaintext_len += len;
        plaintext.resize(plaintext_len);

        EVP_CIPHER_CTX_free(ctx);

        return std::string((char *)plaintext.data(), plaintext_len);
    }

    __attribute__((always_inline)) inline time_t get_dongle()
    {

        time_t now = 0;
        DWORD dwRet = 0;
        int nCount = 0;
        int i = 0;
        int nIndex = -1;
        DWORD dwTime = 0;
        char UserPin[] = "aminer123";
        int nRemainCount = 0;
        DONGLE_INFO *pDongleInfo = NULL;
        DONGLE_HANDLE hDongle = NULL;

        dwRet = Dongle_Enum(NULL, &nCount);
        if (dwRet != 0 || nCount == 0)
        {
            throw std::runtime_error("License Error : No Dongle, Please Insert Usb Key");
        }

        std::cout << "Enum " << nCount << " Dongle ARM.\n";
        pDongleInfo = (DONGLE_INFO *)malloc(nCount * sizeof(DONGLE_INFO));
        if (!pDongleInfo)
        {
            throw std::runtime_error("License Error : Dongle Memory Allocation Failed");
        }
        dwRet = Dongle_Enum(pDongleInfo, &nCount);
        if (dwRet != DONGLE_SUCCESS)
        {
            free(pDongleInfo);
            throw std::runtime_error("License Error : Dongle_Enum Failed");
        }
        for (i = 0; i < nCount; i++)
        {
            if (pDongleInfo[i].m_Type == 0 || pDongleInfo[i].m_Type == 1)
            {
                nIndex = i;
            }
        }

        if (nIndex == -1)
        { // 没有找到时钟锁
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("License Error : Can't Find Dongle, Please Insert Usb Key");
        }
        // 打开锁
        dwRet = Dongle_Open(&hDongle, 0);
        if (DONGLE_SUCCESS != dwRet)
        {
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("License Error : Can't Open Dongle, Please Insert Usb Key");
        }

        // 验证锁用户密码
        dwRet = Dongle_VerifyPIN(hDongle, FLAG_USERPIN, UserPin, &nRemainCount);
        if (DONGLE_SUCCESS != dwRet)
        {
            Dongle_Close(hDongle);
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("License Error : Please Insert Correct Usb Key");
        }

        // 获取锁内时间
        dwRet = Dongle_GetUTCTime(hDongle, &dwTime);
        // printf("Get UTC Time. Return: 0x%08X\n", dwRet);
        // printf("Current Time. ReturnL 0x%08X\n", dwTime);
        if (DONGLE_SUCCESS == dwRet)
        {
            time_t actualTime = dwTime;
            tm *utm = gmtime(&actualTime);
            now = mktime(utm);
        }
        else
        {
            Dongle_Close(hDongle);
            free(pDongleInfo);
            pDongleInfo = NULL;
            throw std::runtime_error("License Error : Get Dongle Time Failed, Please Contact ZHIPU Replace USB Key");
        }
        // 关闭加密锁
        dwRet = Dongle_Close(hDongle);

        if (pDongleInfo != NULL)
        {
            free(pDongleInfo);
            pDongleInfo = NULL;
        }
        return now;
    }

    __attribute__((always_inline)) inline void check_license(const std::filesystem::path &path)
    {

        auto start_time_license = std::chrono::high_resolution_clock::now();
        std::ifstream license_file(std::filesystem::path(path) / "license.txt");
        if (!license_file.is_open())
        {
            throw std::runtime_error("License Error : License File Not Exist!");
        }
        std::ostringstream license_stream;
        license_stream << license_file.rdbuf();
        std::string license = license_stream.str();
        std::string license_decode = base64Decode(license);

        // std::cout << " base64Decode success.\n";
        //  aes decrypt
        std::string key_hex = "d37dffe9718aa504f3624518628b3156b88e8ac8552b4a892ac02cfd93b40c16";
        std::string iv_hex = "20b2af9f545582950f267dec8596d190";
        std::vector<unsigned char> key = hexToBytes(key_hex);
        std::vector<unsigned char> iv = hexToBytes(iv_hex);
        std::vector<unsigned char> ciphertext = hexToBytes(license_decode);
        std::string license_data = aesDecrypt(ciphertext, key, iv);
        // std::cout << "license_data: " << license_data << std::endl;
        license_decode = license_data;

        // std::cout << " aesDecrypt success.\n";

        std::string data = license_decode.substr(0, license_decode.find_last_of('|'));
        std::istringstream ss(license_decode);
        std::string license_tag, license_env, license_endtime, use_dongle, license_signature;

        std::getline(ss, license_tag, '|');
        std::getline(ss, license_env, '|');
        std::getline(ss, license_endtime, '|');
        if (std::count(license_decode.begin(), license_decode.end(), '|') == 4)
        {
            std::getline(ss, use_dongle, '|');
        }
        // std::getline(ss, use_dongle, '|');
        std::getline(ss, license_signature, '|');

        // std::cout << "license_tag: " << license_tag << std::endl;
        // std::cout << "license_env: " << license_env << std::endl;
        // std::cout << "license_endtime: " << license_endtime << std::endl;
        // std::cout << "use_dongle: " << use_dongle << std::endl;
        // std::cout << "license_signature: " << license_signature << std::endl;

        // check license signature
        RSA *public_key = loadPublicKey();
        std::vector<unsigned char> signature = hexToBytes(license_signature);
        // std::cout << "Signature length: "<< signature.size() << std::endl;
        if (!rsaVerify(data, signature, public_key))
        {
            throw std::runtime_error("License Error : Fail To Verify Signature!");
        }

        // std::cout << " signature verify success.\n";

        // check license_endtime
        char *license_admin_pwd = std::getenv("LICENSE_ADMIN_PWD");
        std::string licensePwd = license_admin_pwd ? license_admin_pwd : "NULL";
        std::string currentLicenseAdmin = LICENSE_ADMIN;
        std::cout << "use_dongle: " << use_dongle << std::endl;
        time_t now = 0;

        if ((license_admin_pwd && licensePwd == currentLicenseAdmin) || use_dongle == "false")
        {
            now = time(0);
            std::cout << "Current time: " << std::ctime(&now) << std::endl;
        }
        else
        {
            // consider multi containers access the dongle
            std::filesystem::path dongle_file = path / "use_dongle";
            char *multi_instance = std::getenv("MULTI_INSTANCE");
            if (multi_instance && std::string(multi_instance) == "true")
            {
                if (!std::filesystem::exists(dongle_file))
                {
                    std::ofstream ofs(dongle_file);
                    ofs << "use_dongle=0";
                    ofs.close();
                }
                for (int i = 0; i < 1000; i++)
                {
                    std::ifstream ifs(dongle_file);
                    std::string line;
                    std::getline(ifs, line);
                    if (line == "use_dongle=0")
                    {
                        // 修改为use_dongle=1
                        std::cout << "Begin Occupy Dongle" << std::endl;
                        auto start_time = std::chrono::high_resolution_clock::now();
                        {
                            std::ofstream ofs(dongle_file);
                            ofs << "use_dongle=1";
                        }
                        now = get_dongle();
                        // 修改为use_dongle=0
                        {
                            std::ofstream ofs(dongle_file);
                            ofs << "use_dongle=0";
                        }
                        auto end_time = std::chrono::high_resolution_clock::now();
                        auto duration =
                            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                        std::cout << "End Occupy Dongle" << std::endl;
                        std::cout << "Time taken to occupy dongle: " << duration << " milliseconds" << std::endl;
                        break;
                    }
                    else
                    {
                        // 等待一段时间并重试
                        std::cout << "Dongle is occupied, waiting...\n";
                        // now = time(0);
                        usleep(100000);
                        continue;
                    }
                    throw std::runtime_error("License Error : Get Dongle Failed For MULTI_INSTANCE");
                }
            }
            else
            {
                now = get_dongle();
            }
        }

        //printf("Now time: %ld\n", now);
        //printf("License Expire time: %lld\n", std::stoll(license_endtime));
        if (now > std::stoll(license_endtime))
        {
            throw std::runtime_error("License Error : License Is Expired!");
        }

        // check license_tag

        if (license_tag != LICENSE_TAG)
        {
            throw std::runtime_error("License Error : License Tag Is Not Matched!");
        }

        // check license_env
        if (license_env == "anymachine")
        {
            std::cout << "license is valid!\n";
        }
        else
        {
            std::array<unsigned char, 512> licenseEnv = getEnv();
            std::string info = std::string((char *)licenseEnv.data(), licenseEnv.size());
            info = info.substr(0, info.find('\0'));
            // std::cout << "license is: " << license << "\n";
            std::string delimiter = ";";
            size_t pos = 0;
            std::string token;
            bool found = false;
            while ((pos = license_env.find(delimiter)) != std::string::npos)
            {
                token = license_env.substr(0, pos);
                // std::cout << token << std::endl;
                if (token == info)
                {
                    found = true;
                    break;
                }
                license_env.erase(0, pos + delimiter.length());
            }
            if (!found && license_env != info)
            {
                throw std::runtime_error("License Error : License Env Is Invalid!");
            }
            std::cout << "license is valid!\n";
        }

        auto end_time_license = std::chrono::high_resolution_clock::now();
        auto duration_license =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time_license - start_time_license).count();
        std::cout << "Time taken to verify license: " << duration_license << " milliseconds" << std::endl;
    }
    __attribute__((always_inline)) inline std::string get_nonce() { return std::string(RANDOMS); }

} // namespace glm

PYBIND11_MODULE(license_validator, m) {
    m.def("check_license", [](const std::string &path_str) {
        glm::check_license(std::filesystem::path(path_str));
    }, "Check the license file");
}
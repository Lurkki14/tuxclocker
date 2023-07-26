#include <Crypto.hpp>
#include <openssl/md5.h>
#include <openssl/sha.h>

namespace TuxClocker::Crypto {

std::string sha256(std::string s) {
	auto d = SHA256(reinterpret_cast<const unsigned char *>(s.c_str()), s.size(), 0);

	char out[(SHA256_DIGEST_LENGTH * 2) + 1];

	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		sprintf(out + (i * 2), "%02x", d[i]);

	out[SHA256_DIGEST_LENGTH * 2] = '\0';
	return std::string(out);
}

std::string md5(std::string s) {
	unsigned char data[MD5_DIGEST_LENGTH];
	MD5(reinterpret_cast<const unsigned char *>(s.c_str()), s.size(), data);

	char out[(MD5_DIGEST_LENGTH * 2) + 1];

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(out + (i * 2), "%02x", data[i]);

	out[MD5_DIGEST_LENGTH * 2] = '\0';
	return std::string(out);
}

}; // namespace TuxClocker::Crypto

#include <Crypto.hpp>
#include <openssl/evp.h>
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
	unsigned char data[EVP_MAX_MD_SIZE];
	unsigned int len = 0;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
	EVP_DigestUpdate(ctx, s.c_str(), s.size());
	EVP_DigestFinal_ex(ctx, data, &len);
	EVP_MD_CTX_free(ctx);

	char out[(EVP_MAX_MD_SIZE * 2) + 1];

	for (unsigned int i = 0; i < len; i++)
		sprintf(out + (i * 2), "%02x", data[i]);

	out[len * 2] = '\0';
	return std::string(out);
}

}; // namespace TuxClocker::Crypto

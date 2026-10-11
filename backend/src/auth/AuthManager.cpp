#include "auth/AuthManager.h"

#include <openssl/evp.h>

// This function takes a plain-text password
// and converts it into a SHA-256 hash.
//
// The returned hash is stored as a hexadecimal string.
std::string AuthManager::hashPassword(
    const std::string& password
) {
    // Array used to store the generated hash.
    //
    // EVP_MAX_MD_SIZE is large enough to store
    // the output of supported hashing algorithms.
    unsigned char hash[EVP_MAX_MD_SIZE];

    // This variable will store the actual size
    // of the generated hash.
    unsigned int hashLength = 0;

    // Create a new OpenSSL message-digest context.
    //
    // The context stores the state of the hashing operation.
    EVP_MD_CTX* context = EVP_MD_CTX_new();

    // If the context could not be created,
    // return an empty string to indicate failure.
    if (!context) {
        return "";
    }

    // Initialize the hashing operation using SHA-256.
    EVP_DigestInit_ex(
        context,
        EVP_sha256(),
        nullptr
    );

    // Pass the password data to the SHA-256 algorithm.
    //
    // password.data() gives access to the characters
    // and password.size() tells OpenSSL how many bytes to read.
    EVP_DigestUpdate(
        context,
        password.data(),
        password.size()
    );

    // Finish the hashing operation.
    //
    // The generated hash is stored inside the
    // 'hash' array and its length is stored in 'hashLength'.
    EVP_DigestFinal_ex(
        context,
        hash,
        &hashLength
    );

    // Free the OpenSSL digest context because
    // it is no longer required.
    EVP_MD_CTX_free(context);

    // Characters used to convert each byte of the
    // binary hash into hexadecimal representation.
    static const char hex[] =
        "0123456789abcdef";

    // This string will contain the final hexadecimal hash.
    std::string result;

    // Convert every byte of the binary SHA-256 hash
    // into two hexadecimal characters.
    for (unsigned int i = 0; i < hashLength; ++i) {

        // Extract the upper 4 bits of the byte
        // and convert them into a hexadecimal character.
        result += hex[
            (hash[i] >> 4) & 0x0F
        ];

        // Extract the lower 4 bits of the byte
        // and convert them into a hexadecimal character.
        result += hex[
            hash[i] & 0x0F
        ];
    }

    // Return the hexadecimal representation
    // of the SHA-256 hash.
    return result;
}


// This function checks whether the entered password
// matches the password hash stored in the database.
bool AuthManager::verifyPassword(
    const std::string& password,
    const std::string& storedHash
) {
    // Hash the password entered by the user.
    //
    // If the generated hash is equal to the hash
    // stored in the database, the password is correct.
    return hashPassword(password) == storedHash;
}

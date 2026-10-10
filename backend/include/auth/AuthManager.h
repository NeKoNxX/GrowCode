#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>

// AuthManager is responsible for password-related operations.
//
// It provides:
// 1. Password hashing
// 2. Password verification
class AuthManager {
public:

    // Hashes a plain-text password using SHA-256.
    //
    // The function returns the generated hash
    // as a hexadecimal string.
    static std::string hashPassword(
        const std::string& password
    );

    // Verifies whether a plain-text password
    // matches the hash stored in the database.
    //
    // Returns:
    // true  -> password is correct
    // false -> password is incorrect
    static bool verifyPassword(
        const std::string& password,
        const std::string& storedHash
    );
};

#endif

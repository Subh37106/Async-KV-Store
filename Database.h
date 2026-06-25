#pragma once
#include <unordered_map>
#include <string>
#include <shared_mutex>

class Database {
public:
    // Insert or update a key
    void set(const std::string& key, const std::string& value) {
        store[key] = value;
    }

    // Retrieve a key
    std::string get(const std::string& key) {
        auto it = store.find(key);
        if (it != store.end()) {
            return it->second;
        }
        return "ERROR: Key not found\n";
    }

    // Delete a key
    void del(const std::string& key) {
        store.erase(key);
    }

private:
    std::unordered_map<std::string, std::string> store;
};
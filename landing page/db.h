// Include guard
#ifndef DB_H
#define DB_H

// Include C++ standard libraries
#include <string>
#include <vector>

// DiaryEntry struct
struct DiaryEntry {
    int id;
    std::string title;
    std::string content;
    std::string entry_date;
    std::string created_at;
};

// Function declarations
void createTables();
bool registerUser(const std::string& username, const std::string& password);
bool usernameExists(const std::string& username);
bool resetPassword(const std::string& username, const std::string& newPassword);
int loginUser(const std::string& username, const std::string& password);
bool insertEntry(int user_id, const std::string& title, const std::string& content, const std::string& entry_date);
std::vector<DiaryEntry> fetchEntries(int user_id);
bool updateEntry(int entry_id, const std::string& title, const std::string& content, const std::string& entry_date);
bool deleteEntry(int entry_id, int user_id);
std::vector<DiaryEntry> searchEntries(int user_id, const std::string& keyword);

// End include guard
#endif

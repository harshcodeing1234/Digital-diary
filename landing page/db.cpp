// Includes and namespaces
#include "db.h"
#include <occi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <functional>
using namespace oracle::occi;
using namespace std;

// Environment variable utility
string getEnvVar(const string& key, const string& defaultValue) {
    const char* val = getenv(key.c_str());
    return val ? string(val) : defaultValue;
}

// Password hashing
string hashPassword(const string& password) {
    hash<string> hasher;
    return to_string(hasher(password + "salt_key_2024"));
}

// Database credentials
const string user = getEnvVar("DB_USER", "system");
const string pass = getEnvVar("DB_PASS", "Oracle@123");
const string db   = getEnvVar("DB_HOST", "localhost:1521/orcl");

// Table creation
void createTables() {
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = R"(
        BEGIN
            EXECUTE IMMEDIATE 'CREATE TABLE users (
                id NUMBER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                username VARCHAR2(50) UNIQUE NOT NULL,
                password VARCHAR2(100) NOT NULL
            )';
        EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;)";
        conn->createStatement(sql)->execute();

        sql = R"(
        BEGIN
            EXECUTE IMMEDIATE 'CREATE TABLE entries (
                id NUMBER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
                user_id NUMBER NOT NULL,
                title VARCHAR2(200),
                entry_date DATE,
                content CLOB,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users(id)
            )';
        EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;)";
        conn->createStatement(sql)->execute();

        conn->commit();
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);

        cout << "✅ Tables created or already exist.\n";
    } catch (SQLException& e) {
        cerr << "❌ Table creation error: " << e.getMessage() << endl;
    }
}

// Register user
bool registerUser(const string& username, const string& password) {
    if (username.empty() || password.empty() || username.length() > 50 || password.length() > 100) {
        cerr << "Register Error: Invalid input" << endl;
        return false;
    }
    
    if (usernameExists(username)) {
        cerr << "Register Error: Username already exists" << endl;
        return false;
    }
    
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "INSERT INTO users (username, password) VALUES (:1, :2)";
        Statement* stmt = conn->createStatement(sql);
        stmt->setString(1, username);
        stmt->setString(2, hashPassword(password));

        stmt->executeUpdate();
        conn->commit();

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        return true;
    } catch (SQLException& e) {
        cerr << "Register Error: " << e.getMessage() << endl;
        return false;
    }
}

// Check if username exists
bool usernameExists(const string& username) {
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "SELECT COUNT(*) FROM users WHERE username = :1";
        Statement* stmt = conn->createStatement(sql);
        stmt->setString(1, username);
        
        ResultSet* rs = stmt->executeQuery();
        rs->next();
        int count = rs->getInt(1);

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        
        return count > 0;
    } catch (SQLException& e) {
        cerr << "Username check error: " << e.getMessage() << endl;
        return false;
    }
}

// Reset password
bool resetPassword(const string& username, const string& newPassword) {
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "UPDATE users SET password = :1 WHERE username = :2";
        Statement* stmt = conn->createStatement(sql);
        stmt->setString(1, newPassword);
        stmt->setString(2, username);

        int rows = stmt->executeUpdate();
        conn->commit();

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        
        return rows > 0;
    } catch (SQLException& e) {
        cerr << "Reset password error: " << e.getMessage() << endl;
        return false;
    }
}

// Login user
int loginUser(const string& username, const string& password) {
    if (username.empty() || password.empty()) {
        return -1;
    }
    
    int user_id = -1;
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "SELECT id FROM users WHERE username = :1 AND password = :2";
        Statement* stmt = conn->createStatement(sql);
        stmt->setString(1, username);
        stmt->setString(2, hashPassword(password));

        ResultSet* rs = stmt->executeQuery();
        if (rs->next()) {
            user_id = rs->getInt(1);
        }

        stmt->closeResultSet(rs);
        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
    } catch (SQLException& e) {
        cerr << "Login Error: " << e.getMessage() << endl;
    }
    return user_id;
}

// Insert diary entry
bool insertEntry(int user_id, const string& title, const string& content, const string& entry_date) {
    if (title.empty() || content.empty() || title.length() > 200 || content.length() > 100000) {
        cerr << "Insert Error: Invalid input" << endl;
        return false;
    }
    
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "INSERT INTO entries (user_id, title, content, entry_date) "
                     "VALUES (:1, :2, :3, TO_DATE(:4, 'YYYY-MM-DD'))";
        Statement* stmt = conn->createStatement(sql);
        stmt->setInt(1, user_id);
        stmt->setString(2, title);
        stmt->setString(3, content);
        stmt->setString(4, entry_date);

        int result = stmt->executeUpdate();
        conn->commit();

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        return result > 0;
    } catch (SQLException& e) {
        cerr << "Insert Entry Error: " << e.getMessage() << endl;
        return false;
    }
}

// Read CLOB
static string readClob(Clob& clob) {
    string content;
    if (!clob.isNull()) {
        Stream* instream = clob.getStream();
        char buffer[2048];
        int length;
        while ((length = instream->readBuffer(buffer, sizeof(buffer))) > 0) {
            content.append(buffer, length);
        }
        clob.closeStream(instream);
    }
    return content;
}

// Fetch diary entries
vector<DiaryEntry> fetchEntries(int user_id) {
    vector<DiaryEntry> entries;
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "SELECT id, title, content, "
                     "TO_CHAR(entry_date, 'YYYY-MM-DD'), "
                     "TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') "
                     "FROM entries WHERE user_id = :1 ORDER BY created_at DESC";
        Statement* stmt = conn->createStatement(sql);
        stmt->setInt(1, user_id);

        ResultSet* rs = stmt->executeQuery();
        while (rs->next()) {
            DiaryEntry entry;
            entry.id = rs->getInt(1);
            entry.title = rs->getString(2);

            Clob clob = rs->getClob(3);
            entry.content = readClob(clob);

            entry.entry_date = rs->getString(4);
            entry.created_at = rs->getString(5);
            entries.push_back(entry);
        }

        stmt->closeResultSet(rs);
        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
    } catch (SQLException& e) {
        cerr << "Fetch Entries Error: " << e.getMessage() << endl;
    }
    return entries;
}

// Update diary entry
bool updateEntry(int entry_id, const string& title, const string& content, const string& entry_date) {
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "UPDATE entries SET title = :1, content = :2, "
                     "entry_date = TO_DATE(:3, 'YYYY-MM-DD') WHERE id = :4";
        Statement* stmt = conn->createStatement(sql);
        stmt->setString(1, title);
        stmt->setString(2, content);  // OCCI handles CLOB update as string
        stmt->setString(3, entry_date);
        stmt->setInt(4, entry_id);

        stmt->executeUpdate();
        conn->commit();

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        return true;
    } catch (SQLException& e) {
        cerr << "Update Entry Error: " << e.getMessage() << endl;
        return false;
    }
}

// Delete diary entry
bool deleteEntry(int entry_id, int user_id) {
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "DELETE FROM entries WHERE id = :1 AND user_id = :2";
        Statement* stmt = conn->createStatement(sql);
        stmt->setInt(1, entry_id);
        stmt->setInt(2, user_id);

        int rowsDeleted = stmt->executeUpdate();
        conn->commit();

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
        
        return rowsDeleted > 0;
    } catch (SQLException& e) {
        cerr << "Delete Entry Error: " << e.getMessage() << endl;
        return false;
    }
}

// Search diary entries
vector<DiaryEntry> searchEntries(int user_id, const string& keyword) {
    vector<DiaryEntry> results;
    try {
        Environment* env = Environment::createEnvironment();
        Connection* conn = env->createConnection(user, pass, db);

        string sql = "SELECT id, title, content, "
                     "TO_CHAR(entry_date, 'YYYY-MM-DD'), "
                     "TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') "
                     "FROM entries WHERE user_id = :1 AND "
                     "(LOWER(title) LIKE LOWER(:2) OR LOWER(content) LIKE LOWER(:2)) "
                     "ORDER BY created_at DESC";
        Statement* stmt = conn->createStatement(sql);
        stmt->setInt(1, user_id);
        stmt->setString(2, "%" + keyword + "%");

        ResultSet* rs = stmt->executeQuery();
        while (rs->next()) {
            DiaryEntry entry;
            entry.id = rs->getInt(1);
            entry.title = rs->getString(2);

            Clob clob = rs->getClob(3);
            entry.content = readClob(clob);

            entry.entry_date = rs->getString(4);
            entry.created_at = rs->getString(5);
            results.push_back(entry);
        }

        stmt->closeResultSet(rs);
        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
    } catch (SQLException& e) {
        cerr << "Search Error: " << e.getMessage() << endl;
    }
    return results;
}

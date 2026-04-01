#include "database.hpp"
#include <iostream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

CarbonDatabase::CarbonDatabase(const std::string& path) : db_path(path) {
    fs::path p(path);
    if (!fs::exists(p.parent_path())) {
        fs::create_directories(p.parent_path());
    }

    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db)));
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS daily_carbon ("
                      "date TEXT PRIMARY KEY, "
                      "used REAL DEFAULT 0, "
                      "saved REAL DEFAULT 0);";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        std::string err = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error("SQL error: " + err);
    }
}

CarbonDatabase::~CarbonDatabase() {
    sqlite3_close(db);
}

void CarbonDatabase::log_stats(double used_g, double saved_g) {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    std::string date = oss.str();

    // For now, Veridis provides session-based cumulative.
    // So we store the "latest" cumulative for the day.
    std::string sql = "INSERT INTO daily_carbon (date, used, saved) VALUES ('" + date + "', " 
                    + std::to_string(used_g) + ", " + std::to_string(saved_g) + ") "
                    + "ON CONFLICT(date) DO UPDATE SET used=excluded.used, saved=excluded.saved;";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "Database update error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
}

std::vector<DailyStats> CarbonDatabase::get_history(int days) {
    std::vector<DailyStats> history;
    std::string sql = "SELECT date, used, saved FROM daily_carbon ORDER BY date DESC LIMIT " + std::to_string(days) + ";";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            DailyStats ds;
            ds.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            ds.used_g = sqlite3_column_double(stmt, 1);
            ds.saved_g = sqlite3_column_double(stmt, 2);
            history.push_back(ds);
        }
        sqlite3_finalize(stmt);
    }
    return history;
}

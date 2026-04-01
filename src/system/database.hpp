#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <sqlite3.h>
#include <vector>

struct DailyStats {
    std::string date;
    double used_g;
    double saved_g;
};

class CarbonDatabase {
private:
    sqlite3* db;
    std::string db_path;

public:
    CarbonDatabase(const std::string& path);
    ~CarbonDatabase();
    void log_stats(double used_g, double saved_g);
    std::vector<DailyStats> get_history(int days);
};

#endif

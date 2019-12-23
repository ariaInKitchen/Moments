
#include "DatabaseHelper.h"
#include <sstream>


#define DATABASE_FILE   "moments.db"
#define SETTING_TABLE   "moments_setting"
#define LIST_TABLE     "moments_list"

namespace elastos {

DatabaseHelper::DatabaseHelper(const std::string& path)
{
    std::stringstream ss(path);
    ss << "/" << DATABASE_FILE;
    int ret = sqlite3_open(ss.str().c_str(), &mDb);
    if (ret != SQLITE_OK) {
        printf("open database %s failed, error code: %d\n", ss.str().c_str(), ret);
        return;
    }

    bool exist = TableExist(SETTING_TABLE);
    if (!exist) {
        CreateSettingTable();
    }

    exist = TableExist(LIST_TABLE);
    if (!exist) {
        CreateDataTable();
    }
}

DatabaseHelper::~DatabaseHelper()
{
    sqlite3_close(mDb);
}

bool DatabaseHelper::TableExist(const std::string& name)
{
    std::stringstream ss;
    ss << "select count(*) from sqlite_master where type='table' and name='" << name << "'";

    sqlite3_stmt* pStmt = nullptr;
    bool exist = false;
    int count;
    int ret = sqlite3_prepare_v2(mDb, ss.str().c_str(), -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        goto exit;
    }

    ret = sqlite3_step(pStmt);
    if (SQLITE_OK != ret && SQLITE_DONE != ret && SQLITE_ROW != ret) {
        goto exit;
    }

    count = sqlite3_column_int(pStmt, 0);
    exist = count > 0;

exit:
    if (pStmt) {
        sqlite3_finalize(pStmt);
    }
    return exist;
}

int DatabaseHelper::CreateSettingTable()
{
    std::stringstream ss;
    ss << "CREATE TABLE " << SETTING_TABLE << "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, ";
    ss << "name TEXT UNIQUE NOT NULL, value TEXT NOT NULL);";

    return CreateTable(ss.str());
}

int DatabaseHelper::CreateDataTable()
{
    std::stringstream ss;
    ss << "CREATE TABLE " << SETTING_TABLE << "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, ";
    ss << "type INTEGER NOT NULL, content TEXT NOT NULL, time INTEGER NOT NULL, files TEXT, access TEXT);";

    return CreateTable(ss.str());
}

int DatabaseHelper::CreateTable(const std::string& sql)
{
    char* errMsg;
    int ret = sqlite3_exec(mDb, sql.c_str(), NULL, NULL, &errMsg);
    if (ret !=  SQLITE_OK) {
        printf("create table failed ret %d, %s\n", ret, errMsg);
        sqlite3_free(errMsg);
    }

    return ret;
}

int DatabaseHelper::SetOwner(const std::string& owner)
{
    std::stringstream stream;
    stream << "INSERT OR REPLACE INTO '" << SETTING_TABLE;
    stream << "'(id,name,value) VALUES (";
    stream << "(SELECT id FROM '" << SETTING_TABLE << "' WHERE name='owner'),'";
    stream << "owner','" << owner << "');";

    return Insert(stream.str());
}

std::string DatabaseHelper::GetOwner()
{
    char sql[512];
    sqlite3_stmt* pStmt = nullptr;
    std::stringstream ss;
    ss << "SELECT * FROM '" << SETTING_TABLE << "' WHERE name='owner';";

    char* owner;
    int ret = sqlite3_prepare_v2(mDb, ss.str().c_str(), -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        goto exit;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_OK) {
        goto exit;
    }

    owner = (char*)sqlite3_column_text(pStmt, 2);

exit:
    if (pStmt) {
        sqlite3_finalize(pStmt);
    }
    return owner;
}

int DatabaseHelper::SetPrivate(bool priv)
{
    std::stringstream stream;
    stream << "INSERT OR REPLACE INTO '" << SETTING_TABLE;
    stream << "'(id,name,value) VALUES (";
    stream << "(SELECT id FROM '" << SETTING_TABLE << "' WHERE name='access'),'";
    stream << "access','" << (priv ? "private" : "public") << "');";

    return Insert(stream.str());
}

bool DatabaseHelper::GetPrivate()
{
    char sql[512];
    sqlite3_stmt* pStmt = nullptr;
    std::stringstream ss;
    ss << "SELECT * FROM '" << SETTING_TABLE << "' WHERE name='access';";

    char* priv;
    bool isPrivate = false;
    int ret = sqlite3_prepare_v2(mDb, ss.str().c_str(), -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        goto exit;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_OK) {
        goto exit;
    }

    priv = (char*)sqlite3_column_text(pStmt, 2);
    if (!strcmp(priv, "private")) {
        isPrivate = true;
    }

exit:
    if (pStmt) {
        sqlite3_finalize(pStmt);
    }
    return isPrivate;
}

int DatabaseHelper::InsertData(int type, const std::string& content,
            long time, const std::string& files, const std::string& access)
{
    std::stringstream stream;
    stream << "INSERT INTO '" << LIST_TABLE;
    stream << "'(type,content,time,files,access) VALUES (";
    stream << type << ",'" << content << "'," << time;
    stream << ",'" << files << "','" << access << "');";

    int ret = Insert(stream.str());
    if (ret != SQLITE_OK) {
        return ret;
    }

    int id = sqlite3_last_insert_rowid(mDb);
    return id;
}

int DatabaseHelper::RemoveData(int id)
{
    char* errMsg;
    std::stringstream ss;
    ss << "DELETE FROM '" << LIST_TABLE << "' WHERE id=" << id << ";";

    int ret = sqlite3_exec(mDb, ss.str().c_str(), NULL, NULL, &errMsg);
    if (ret != SQLITE_OK) {
        printf("remove data id %d failed ret %d, %s\n", id, ret, errMsg);
        sqlite3_free(errMsg);
    }

    return ret;
}

int DatabaseHelper::ClearData()
{
    char* errMsg;
    std::stringstream ss;
    ss << "DELETE FROM '" << LIST_TABLE << "';";

    int ret = sqlite3_exec(mDb, ss.str().c_str(), NULL, NULL, &errMsg);
    if (ret != SQLITE_OK) {
        printf("clear data failed ret %d, %s\n", ret, errMsg);
        sqlite3_free(errMsg);
        return ret;
    }

    ret = sqlite3_exec(mDb, "VACUUM;", NULL, NULL, &errMsg);
    if (ret != SQLITE_OK) {
        printf("clear unused space failed ret %d, %s\n", ret, errMsg);
        sqlite3_free(errMsg);
    }

    return ret;
}

int DatabaseHelper::Insert(const std::string& sql)
{
    char* errMsg;
    int ret = sqlite3_exec(mDb, "BEGIN;", NULL, NULL, &errMsg);
    if (ret != SQLITE_OK) {
        printf("insert begin transaction failed ret %d, %s\n", ret, errMsg);
        sqlite3_free(errMsg);
        return ret;
    }

    printf("insert sql: %s\n", sql.c_str());
    ret = sqlite3_exec(mDb, sql.c_str(), NULL, NULL, &errMsg);
    if (ret != SQLITE_OK) {
        printf("insert failed ret %d, %s\n", ret, errMsg);
        sqlite3_free(errMsg);
        sqlite3_exec(mDb, "ROLLBACK;", NULL, NULL, NULL);
        return ret;
    }

    ret = sqlite3_exec(mDb, "COMMIT;", NULL, NULL, NULL);
    return ret;
}

int DatabaseHelper::GetData(long time, std::stringstream& data)
{
    sqlite3_stmt* pStmt = nullptr;
    int ret = 0;
    std::stringstream ss;
    bool first = true;
    ss << "SELECT * FROM '" << LIST_TABLE << "'";
    if (time <= 0) {
        ss << " WHERE time>" << time;
    }
    ss << " ORDER BY time DESC;";

    ret = sqlite3_prepare_v2(mDb, ss.str().c_str(), -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        printf("Get data prepare failed ret:%d\n", ret);
        goto exit;
    }

    data << "[";

    while(SQLITE_ROW == sqlite3_step(pStmt)) {
        if (!first) {
            data << ",";
        }
        else {
            first = false;
        }
        data << "{";
        int id = sqlite3_column_int(pStmt, 0);
        data << "'id':" << id <<",";

        int type = sqlite3_column_int(pStmt, 1);
        data << "'type':" << type <<",";

        char* content = (char*)sqlite3_column_text(pStmt, 2);
        data << "'content':'" << content <<"',";

        long recordTime = sqlite3_column_int64(pStmt, 3);
        data << "'time':" << recordTime <<",";

        char* files = (char*)sqlite3_column_text(pStmt, 4);
        data << "'files':'" << files <<"',";

        char* access = (char*)sqlite3_column_text(pStmt, 5);
        data << "'access':'" << access <<"'}";
    }

    data << "]";

exit:
    if (pStmt) {
        sqlite3_finalize(pStmt);
    }
    return ret;
}

}

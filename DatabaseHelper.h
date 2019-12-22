
#ifndef __ELASTOS_DATABASE_HELPER_H__
#define __ELASTOS_DATABASE_HELPER_H__

#include <sqlite3.h>
#include <string>

namespace elastos {

class DatabaseHelper
{
public:
    DatabaseHelper(const std::string& path);
    ~DatabaseHelper();

    int SetOwner(const std::string& owner);

    std::string GetOwner();

    int SetPrivate(bool priv);

    bool GetPrivate();

    int InsertData(int type, const std::string& content,
                long time, const std::string& files, const std::string& access);

    int RemoveData(int id);

    int ClearData();

    int GetData(long time, std::stringstream& data);

private:
    bool TableExist(const std::string& name);

    int CreateSettingTable();
    int CreateDataTable();

    int CreateTable(const std::string& sql);

    int Insert(const std::string& sql);

private:
    sqlite3* mDb;
};

}

#endif //__ELASTOS_DATABASE_HELPER_H__

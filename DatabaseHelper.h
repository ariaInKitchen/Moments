
#ifndef __ELASTOS_DATABASE_HELPER_H__
#define __ELASTOS_DATABASE_HELPER_H__

#include <sqlite3.h>
#include <string>
#include "Json.hpp"

#define DATA_LIMIT  5

namespace elastos {

class DatabaseHelper
{
public:
    class Moment
    {
    public:
        Moment(int id, int type, const std::string& content,
            long time, const std::string& files, const std::string& access)
            : mId(id)
            , mType(type)
            , mContent(content)
            , mTime(time)
            , mFiles(files)
            , mAccess(access)
        {}

        Json toJson();
        std::string toString();

    private:
        int mId;
        int mType;
        std::string mContent;
        long mTime;
        std::string mFiles;
        std::string mAccess;
    };

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

    int GetData(long time, std::stringstream& data, long* lastTime);

    int GetData(long time, Json& json);

    std::shared_ptr<DatabaseHelper::Moment> GetData(int id);

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

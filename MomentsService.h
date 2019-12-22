
#ifndef __ELASTOS_MOMENTS_SERVICE_H__
#define __ELASTOS_MOMENTS_SERVICE_H__

#include <string>
#include <memory>
#include "Connector.h"
#include "DatabaseHelper.h"

#define MOMENTS_SERVICE_NAME    "moments"

namespace elastos  {

class MomentsService
{
public:
    MomentsService(const std::string& path);
    ~MomentsService() = default;

    int SetOwner(const std::string& owner);
    std::string GetOwner();

    // Apis for Owner
    int SetPrivate(bool priv);
    bool IsPrivate();

    int Add(int type, const std::string& content,
            long time, const std::string& files, const std::string& access);
    int Remove(int id);

    int Clear();

    // Apis for others
    int Comment(const std::string& friendCode, const std::string& content);

private:
    int UpdateFriendList(const std::string& friendCode, const FriendInfo::Status& status);

    void UserStatusChanged(const FriendInfo::Status& status);

private:
    std::string mPath;
    std::string mOwner;
    std::string mUserCode;
    bool mPrivate;

    std::shared_ptr<Connector> mConnector;
    std::shared_ptr<DatabaseHelper> mDbHelper;

    std::vector<std::shared_ptr<ElaphantContact::FriendInfo>> mOnlineFriendList;

    friend class MomentsListener;
};

}

#endif //__ELASTOS_MOMENTS_SERVICE_H__

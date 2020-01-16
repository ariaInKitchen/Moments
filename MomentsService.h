
#ifndef __ELASTOS_MOMENTS_SERVICE_H__
#define __ELASTOS_MOMENTS_SERVICE_H__

#include <string>
#include <memory>
#include <thread>
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

    void StartMessageThread();
    void StopMessageThread();

    void NotifyPushMessage();

    void PushMoments(std::shared_ptr<ElaphantContact::FriendInfo>& friendInfo);

    void SendSetting(const std::string& type);
    void SendData(const std::string& friendCode, int id);
    void SendDataList(const std::string& friendCode, long time);

    bool IsDid(const std::string& friendCode);

    void PublishResponse(long time, int result);
    void DeleteResponse(int id, int result);
    void ClearResponse(int result);
    void SettingResponse(const std::string& type, int result);

    void SendFollowList(const std::string& friendCode);
    void SendNewFollow(const std::string& friendCode);

    static void ThreadFun(MomentsService* service);

private:
    std::string mPath;
    std::string mOwner;
    std::string mUserCode;
    bool mPrivate;

    std::shared_ptr<Connector> mConnector;
    std::shared_ptr<DatabaseHelper> mDbHelper;

    std::mutex mListMutex;
    std::vector<std::shared_ptr<ElaphantContact::FriendInfo>> mOnlineFriendList;

    // condition varialbe wait
    std::condition_variable mCv;
    std::mutex mCvMutex;

    std::shared_ptr<std::thread> mMessageThread;

    bool mStopThread;

    friend class MomentsListener;
};

}

#endif //__ELASTOS_MOMENTS_SERVICE_H__

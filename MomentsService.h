
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

    void NotifyNewMessage();

    void PushMoment(const std::string& friendCode, const std::shared_ptr<Moment>& moment);

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
    std::vector<std::shared_ptr<Moment>> mMsgList;

    // condition varialbe wait
    std::condition_variable mCv;
    std::mutex mCvMutex;

    std::shared_ptr<std::thread> mMessageThread;

    bool mStopThread;

    friend class MomentsListener;
};

}

#endif //__ELASTOS_MOMENTS_SERVICE_H__

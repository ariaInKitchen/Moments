
#include "MomentsService.h"
#include "MomentsListener.h"
#include "ghc-filesystem.hpp"

namespace elastos {

extern "C" {

void* CreateService(const char* path)
{
    MomentsService* service = new MomentsService(path);
    return static_cast<void *>(service);
}

void DestroyService(void* service)
{
    if (service != nullptr) {
        MomentsService* instance = static_cast<MomentsService *>(service);
        delete instance;
    }
}

}

std::string MomentsService::Moment::toString()
{
    Json json;
    json["id"] = mId;
    json["type"] = mType;
    json["content"] = mContent;
    json["time"] = mTime;
    json["files"] = mFiles;
    json["access"] = mAccess;

    return json.dump();
}

MomentsService::MomentsService(const std::string& path)
    : mPath(path)
    , mStopThread(true)
{
    mConnector = std::make_shared<Connector>(MOMENTS_SERVICE_NAME);
    auto listener = std::shared_ptr<PeerListener::MessageListener>(new MomentsListener(this));
    mConnector->SetMessageListener(listener);

    std::shared_ptr<ElaphantContact::UserInfo> userInfo = mConnector->GetUserInfo();
    mPath.append("/");
    userInfo->getHumanCode(mUserCode);
    mPath.append(mUserCode);
    mPath.append("/Moments");

    std::error_code stdErrCode;
    bool ret = ghc::filesystem::create_directories(mPath, stdErrCode);
    if(ret == false || stdErrCode.value() != 0) {
        int errCode = ErrCode::StdSystemErrorIndex - stdErrCode.value();
        auto errMsg = ErrCode::ToString(errCode);
        printf("MomentsService Failed to set local data dir, errcode: %s", errMsg.c_str());
    }

    mDbHelper = std::make_shared<DatabaseHelper>(mPath);
    mOwner = mDbHelper->GetOwner();
    mPrivate = mDbHelper->GetPrivate();

    printf("MomentsService owner %s isPirvate %d\n", mOwner.c_str(), mPrivate);

    const auto& friendList = mConnector->ListFriendInfo();
    if (mOwner.empty() && !friendList.empty()) {
        auto first = friendList[0];
        std::string friendCode;
        first->getHumanCode(friendCode);
        SetOwner(friendCode);
    }
}

int MomentsService::SetOwner(const std::string& owner)
{
    mOwner = owner;
    return mDbHelper->SetOwner(mOwner);
}

std::string MomentsService::GetOwner()
{
    return mOwner;
}

int MomentsService::SetPrivate(bool priv)
{
    mPrivate = priv;

    return mDbHelper->SetPrivate(mPrivate);
}

bool MomentsService::IsPrivate()
{
    return mPrivate;
}

int MomentsService::Add(int type, const std::string& content,
            long time, const std::string& files, const std::string& access)
{
    int ret = mDbHelper->InsertData(type, content, time, files, access);
    if (ret > 0) {
        printf("insert to db id %d\n", ret);
        auto moment = std::make_shared<Moment>(ret, type, content, time, files, access);
        std::unique_lock<std::mutex> _lock(mListMutex);
        mMsgList.push_back(moment);
        NotifyNewMessage();
    }

    return ret;
}

int MomentsService::Remove(int id)
{
    return mDbHelper->RemoveData(id);
}

int MomentsService::Clear()
{
    return mDbHelper->ClearData();
}

int MomentsService::Comment(const std::string& friendCode, const std::string& content)
{
    return -1;
}

int MomentsService::UpdateFriendList(const std::string& friendCode, const FriendInfo::Status& status)
{
    if (!friendCode.compare(mOwner)) {
        printf("MomentsService owner status changed\n");
        return 0;
    }

    std::unique_lock<std::mutex> _lock(mListMutex);
    std::shared_ptr<ElaphantContact::FriendInfo> friendInfo;
    mConnector->GetFriendInfo(friendCode, friendInfo);
    if (status == FriendInfo::Status::Online) {
        mOnlineFriendList.push_back(friendInfo);
    }
    else {
        for (auto it = mOnlineFriendList.begin(); it != mOnlineFriendList.end(); it++) {
            std::string humanCode;
            (*it)->getHumanCode(humanCode);
            if (!friendCode.compare(humanCode)) {
                mOnlineFriendList.erase(it);
                break;
            }
        }
    }

    return 0;
}

void MomentsService::UserStatusChanged(const FriendInfo::Status& status)
{
    if (status == FriendInfo::Status::Online) {
        StartMessageThread();
    }
    else {
        StopMessageThread();
    }
}

void MomentsService::StartMessageThread()
{
    if (mMessageThread.get() != nullptr) return;

    mStopThread = false;
    mMessageThread = std::make_shared<std::thread>(MomentsService::ThreadFun, this);
}

void MomentsService::StopMessageThread()
{
    mStopThread = true;
    mMessageThread->join();
    mMessageThread.reset();
}

void MomentsService::NotifyNewMessage()
{
    std::unique_lock<std::mutex> lk(mCvMutex);
    lk.unlock();
    mCv.notify_one();
}

void MomentsService::PushMoment(const std::string& friendCode, const std::shared_ptr<Moment>& moment)
{
    printf("MomentsService push moment to %s\n", friendCode.c_str());
    Json json;
    json["serviceName"] = MOMENTS_SERVICE_NAME;
    Json content;
    content["command"] = "pushData";
    content["content"] = moment->toString();
    json["content"] = content;

    mConnector->SendMessage(friendCode, json.dump());
}

void MomentsService::SendSetting(const std::string& type)
{
    if (!type.compare("access")) {
        Json json;
        json["serviceName"] = MOMENTS_SERVICE_NAME;
        Json content;
        content["command"] = "getSetting";
        content["type"] = type;
        json["value"] = mPrivate;
        mConnector->SendMessage(mOwner, json.dump());
    }
    else {
        printf("MomentsService do not support this type: %s\n", type.c_str());
    }
}

void MomentsService::ThreadFun(MomentsService* service)
{
    printf("Moments service start message thread.\n");

    while (!service->mStopThread) {
        printf("Momtents service message thread wait\n");
        std::unique_lock<std::mutex> lk(service->mCvMutex);
        service->mCv.wait(lk);

        printf("Momtents service message thread aweak\n");
        std::unique_lock<std::mutex> _lock(service->mListMutex);
        if (service->mOnlineFriendList.size() == 0 || service->mMsgList.size() == 0) {
            continue;
        }

        for (auto it = service->mMsgList.begin(); it != service->mMsgList.end(); it++) {
            for (const auto& friendItem : service->mOnlineFriendList) {
                std::string humanCode;
                friendItem->getHumanCode(humanCode);
                service->PushMoment(humanCode, *it);
            }
        }
        service->mMsgList.clear();
    }

    printf("Moments service stop message thread.\n");
}

}

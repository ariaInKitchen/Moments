
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

MomentsService::MomentsService(const std::string& path)
    : mPath(path)
    , mStopThread(true)
{
    mConnector = std::make_shared<Connector>(MOMENTS_SERVICE_NAME);
    auto listener = std::shared_ptr<PeerListener::MessageListener>(new MomentsListener(this));
    mConnector->SetMessageListener(listener);

    std::shared_ptr<ElaphantContact::UserInfo> userInfo = mConnector->GetUserInfo();
    userInfo->getHumanCode(mUserCode);
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
        if (IsDid(friendCode)) {
            SetOwner(friendCode);
        }
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
        NotifyPushMessage();
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
        NotifyPushMessage();
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

void MomentsService::NotifyPushMessage()
{
    std::unique_lock<std::mutex> lk(mCvMutex);
    lk.unlock();
    mCv.notify_one();
}

void MomentsService::PushMoments(std::shared_ptr<ElaphantContact::FriendInfo>& friendInfo)
{
    std::string humanCode;
    friendInfo->getHumanCode(humanCode);
    printf("MomentsService push moments to %s\n", humanCode.c_str());

    long time = 0;
    std::string addition;
    friendInfo->getHumanInfo(ElaphantContact::HumanInfo::Item::Addition, addition);
    if (!addition.empty()) {
        time = std::stol(addition);
    }

    long lastTime;
    std::stringstream ss;
    int ret = mDbHelper->GetData(time, ss, &lastTime);
    if (ret != SQLITE_OK) {
        printf("get data error \n");
        return;
    }

    std::string record = ss.str();
    if (record.size() <= 5) {
        printf("no new moment\n");
        return;
    }

    Json content;
    content["command"] = "pushData";
    content["content"] = record;

    ret = mConnector->SendMessage(humanCode, content.dump());
    if (ret != 0) return;

    friendInfo->setHumanInfo(ElaphantContact::HumanInfo::Item::Addition, std::to_string(lastTime));
}

void MomentsService::SendSetting(const std::string& type)
{
    if (!type.compare("access")) {
        Json content;
        content["command"] = "getSetting";
        content["type"] = type;
        content["value"] = mPrivate;
        mConnector->SendMessage(mOwner, content.dump());
    }
    else {
        printf("MomentsService do not support this type: %s\n", type.c_str());
    }
}

void MomentsService::SendData(const std::string& friendCode, int id)
{
    if (id < 0) return;
    auto moment = mDbHelper->GetData(id);
    if (moment == nullptr) {
        printf("MomentsService data id %d not found\n", id);
        return;
    }

    Json content;
    content["command"] = "getData";
    content["content"] = moment->toJson();

    mConnector->SendMessage(friendCode, content.dump());
}

void MomentsService::SendDataList(const std::string& friendCode, long time)
{
    Json moments = Json::array();
    int ret = mDbHelper->GetData(time, moments);
    if (ret != SQLITE_OK) {
        printf("MomentsService GetData failed %d\n", ret);
        return;
    }
    if (moments.size() == 0) {
        printf("MomentsService GetData no new data\n");
        return;
    }

    Json content;
    content["command"] = "getDataList";
    content["content"] = moments;

    mConnector->SendMessage(friendCode, content.dump());
}

bool MomentsService::IsDid(const std::string& friendCode)
{
    if (friendCode.size() == 34 && friendCode.at(0) == 'i') return true;
    else return false;
}

void MomentsService::PublishResponse(long time, int result)
{
    Json content;
    content["command"] = "publish";
    content["time"] = time;
    content["result"] = result;

    mConnector->SendMessage(mOwner, content.dump());
}

void MomentsService::DeleteResponse(int id, int result)
{
    Json content;
    content["command"] = "delete";
    content["id"] = id;
    content["result"] = result;

    mConnector->SendMessage(mOwner, content.dump());
}

void MomentsService::ClearResponse(int result)
{
    Json content;
    content["command"] = "clear";
    content["result"] = result;

    mConnector->SendMessage(mOwner, content.dump());
}

void MomentsService::SettingResponse(const std::string& type, int result)
{
    Json content;
    content["command"] = "setting";
    content["type"] = type;
    content["value"] = mPrivate;
    content["result"] = result;

    mConnector->SendMessage(mOwner, content.dump());
}

void MomentsService::SendFollowList(const std::string& friendCode)
{
    const auto& friendList = mConnector->ListFriendInfo();

    Json content;
    content["command"] = "getFollowList";

    Json list = Json::array();
    int index = 0;
    for (auto friendInfo : friendList) {
        std::string humanCode;
        friendInfo->getHumanCode(humanCode);
        if (humanCode.compare(mOwner)) {
            list[index] = humanCode;
            index++;
        }
    }

    content["content"] = list;

    mConnector->SendMessage(mOwner, content.dump());
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
        if (service->mOnlineFriendList.size() == 0) {
            continue;
        }

        for (auto& friendItem : service->mOnlineFriendList) {
            service->PushMoments(friendItem);
        }
    }

    printf("Moments service stop message thread.\n");
}

}


#include "MomentsService.h"
#include "MomentsListener.h"

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
{
    mConnector = std::make_shared<Connector>(MOMENTS_SERVICE_NAME);
    auto listener = std::shared_ptr<PeerListener::MessageListener>(new MomentsListener(this));
    mConnector->SetMessageListener(listener);

    std::shared_ptr<ElaphantContact::UserInfo> userInfo = mConnector->GetUserInfo();
    mPath.append("/");
    userInfo->getHumanCode(mUserCode);
    mPath.append(mUserCode);
    mPath.append("/Moments");

    mDbHelper = std::make_shared<DatabaseHelper>(mPath);
    mOwner = mDbHelper->GetOwner();
    mPrivate = mDbHelper->GetPrivate();

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
    return mDbHelper->InsertData(type, content, time, files, access);
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
    std::shared_ptr<ElaphantContact::FriendInfo> friendInfo;
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
        // start notify thread
    }
    else {
        // stop notify thread
    }
}

}

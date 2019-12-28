
#include "MomentsListener.h"
#include "Json.hpp"

namespace elastos {

void MomentsListener::onEvent(ElaphantContact::Listener::EventArgs& event)
{
    switch (event.type) {
    case ElaphantContact::Listener::EventType::StatusChanged:
    {
        auto statusEvent = dynamic_cast<ElaphantContact::Listener::StatusEvent*>(&event);
        printf("Serice %s received %s status changed %hhu\n", MOMENTS_SERVICE_NAME, event.humanCode.c_str(), statusEvent->status);
        HandleStatusChanged(statusEvent);
        break;
    }
    case ElaphantContact::Listener::EventType::FriendRequest:
    {
        auto requestEvent = dynamic_cast<ElaphantContact::Listener::RequestEvent*>(&event);
        printf("Serice %s received %s friend request %s\n", MOMENTS_SERVICE_NAME, event.humanCode.c_str(), requestEvent->summary.c_str());
        HandleFriendRequest(requestEvent);
        break;
    }
    case ElaphantContact::Listener::EventType::HumanInfoChanged:
    {
        auto infoEvent = dynamic_cast<ElaphantContact::Listener::InfoEvent*>(&event);
        std::string content = infoEvent->toString();
        printf("Serice %s received %s info changed %s\n", MOMENTS_SERVICE_NAME, event.humanCode.c_str(), content.c_str());
        break;
    }
    default:
        printf("Unprocessed event: %d", static_cast<int>(event.type));
        break;
    }
}

void MomentsListener::onReceivedMessage(const std::string& humanCode, ElaphantContact::Channel channelType,
                               std::shared_ptr<ElaphantContact::Message> msgInfo)
{
    printf("Service %s received message %s from %s\n", MOMENTS_SERVICE_NAME, msgInfo->data->toString().c_str(), humanCode.c_str());
    try {
        Json json = Json::parse(msgInfo->data->toString());

        Json content = json["content"];
        std::string command = content["command"];

        if (!command.compare("setting")) {
            HandleSetting(humanCode, content);
        }
        else if (!command.compare("getData")) {
            HandleGetData(humanCode, content);
        }
        else if (!command.compare("getDataList")) {
            HandleGetDataList(humanCode, content);
        }
        else if (!command.compare("delete")) {
            HandleDelete(humanCode, content);
        }
        else if (!command.compare("clear")) {
            HandleClear(humanCode);
        }
        else if (!command.compare("getSetting")) {
            HandleGetSetting(humanCode, content);
        }
        else if (!command.compare("publish")) {
            HandlePublish(humanCode, content);
        }
        else if (!command.compare("acceptFriend")) {
            AcceptFriend(humanCode, content);
        }
        else if (!command.compare("getFollowList")) {
            HandleGetFollowList(humanCode);
        }
        else {
            printf("Not support command %s\n", command.c_str());
        }

    } catch (const std::exception& e) {
        printf("Service moment parse json failed\n");
    }
}

void MomentsListener::HandleFriendRequest(ElaphantContact::Listener::RequestEvent* event)
{
    if (mService->mPrivate) {
        Json content;
        content["command"] = "friendRequest";
        content["friendCode"] = event->humanCode;
        Json summary = Json::parse(event->summary);
        content["summary"] = summary["content"];
        Json json;
        json["serviceName"] = MOMENTS_SERVICE_NAME;
        json["content"] = content;

        mService->mConnector->SendMessage(mService->mOwner, json.dump());
    }
    else {
        if (mService->mOwner.empty()) {
            // only did user can be owner
            if (!mService->IsDid(event->humanCode)) return;
            mService->SetOwner(event->humanCode);
        }

        mService->mConnector->AcceptFriend(event->humanCode);
    }
}

void MomentsListener::HandleStatusChanged(ElaphantContact::Listener::StatusEvent* event)
{
    if (!mService->mUserCode.compare(event->humanCode)) {
        mService->UserStatusChanged(event->status);
    }
    else {
        mService->UpdateFriendList(event->humanCode, event->status);
    }
}

void MomentsListener::HandleSetting(const std::string& humanCode, const Json& json)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }
    std::string type = json["type"];
    if (!type.compare("access")) {
        bool priv = json["value"];
        int ret = mService->SetPrivate(priv);
        mService->SettingResponse("access", ret);
    }
}

void MomentsListener::HandleDelete(const std::string& humanCode, const Json& json)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }

    int id = json["id"];
    int ret = mService->Remove(id);
    mService->DeleteResponse(id, ret);
}

void MomentsListener::HandleClear(const std::string& humanCode)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }
    int ret = mService->Clear();
    mService->ClearResponse(ret);
}

void MomentsListener::HandlePublish(const std::string& humanCode, const Json& json)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }

    int type = json["type"];
    std::string content = json["content"];
    long time = json["time"];
    std::string access = json["access"];
    int ret = mService->Add(type, content, time, "", access);
    mService->PublishResponse(time, ret);
}

void MomentsListener::AcceptFriend(const std::string& humanCode, const Json& json)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }
    std::string friendCode = json["friendCode"];
    mService->mConnector->AcceptFriend(friendCode);
}

void MomentsListener::HandleGetSetting(const std::string& humanCode, const Json& json)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }

    std::string type = json["type"];
    mService->SendSetting(type);
}

void MomentsListener::HandleGetData(const std::string& humanCode, const Json& json)
{
    int id = json["id"];
    mService->SendData(humanCode, id);
}

void MomentsListener::HandleGetDataList(const std::string& humanCode, const Json& json)
{
    long time = json["time"];
    mService->SendDataList(humanCode, time);
}

void MomentsListener::HandleGetFollowList(const std::string& humanCode)
{
    if (humanCode.compare(mService->mOwner)) {
        printf("This is an owner command\n");
        return;
    }
    mService->SendFollowList(humanCode);
}

}

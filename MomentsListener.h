
#ifndef __ELASTOS_MOMENTS_LISTENER_H__
#define __ELASTOS_MOMENTS_LISTENER_H__

#include "PeerListener.h"
#include "MomentsService.h"

namespace elastos {

class MomentsListener : public PeerListener::MessageListener
{
public:
    MomentsListener(MomentsService* service)
        : mService(service)
    {}

    ~MomentsListener() = default;

    virtual void onEvent(ElaphantContact::Listener::EventArgs& event) override;
    virtual void onReceivedMessage(const std::string& humanCode, ElaphantContact::Channel channelType,
                               std::shared_ptr<ElaphantContact::Message> msgInfo) override;

private:
    void HandleFriendRequest(ElaphantContact::Listener::RequestEvent* event);
    void HandleStatusChanged(ElaphantContact::Listener::StatusEvent* event);

    void HandleSetting(const std::string& humanCode, const Json& json);
    void HandleDelete(const std::string& humanCode, const Json& json);
    void HandleClear(const std::string& humanCode);
    void HandlePublish(const std::string& humanCode, const Json& json);
    void AcceptFriend(const std::string& humanCode, const Json& json);

    void HandleGetSetting(const std::string& humanCode, const Json& json);

private:
    MomentsService* mService;
};

}

#endif //__ELASTOS_MOMENTS_LISTENER_H__

//
// Created by sajith on 12/6/21.
//

#include "shared_state.h"
#include "websocket_session.h"

shared_state::shared_state(std::string doc_root) : doc_root_(std::move(doc_root))
{

}

void shared_state::join(websocket_session *session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);
}

void shared_state::leave(websocket_session *session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}


// Broadcast a message to all websocket clients
void shared_state::send(std::string message)
{
    auto const ss = boost::make_shared<std::string const>(std::move(message));
    std::vector<boost::weak_ptr<websocket_session>> v;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        v.reserve(sessions_.size());
        for (auto p: sessions_)
        {
            v.emplace_back(p->weak_from_this());
        }
    }

    for (auto const &wp: v)
    {
        if (auto sp = wp.lock())
        {
            sp->send(ss);
        }
    }
}


//
// Created by sajith on 12/6/21.
//

#ifndef BOOSTCHATSERVER_SHARED_STATE_H
#define BOOSTCHATSERVER_SHARED_STATE_H

#include <boost/smart_ptr.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

// Forward declaration

class websocket_session;

class shared_state
{
    std::string const doc_root_;

    // This mutex synchronizes all access to sessions_
    std::mutex mutex_;
    // Keep a list of all the connected clients
    std::unordered_set<websocket_session> sessions_;

public:
    explicit shared_state(std::string doc_root);

    std::string const &doc_root() const noexcept
    {
        return doc_root_;
    }

    void join(websocket_session *session);
    void leave(websocket_session *session);
    void send(std::string message);
};


#endif //BOOSTCHATSERVER_SHARED_STATE_H

//
// Created by sajith on 12/6/21.
//

#ifndef BOOSTCHATSERVER_LISTENER_H
#define BOOSTCHATSERVER_LISTENER_H

#include "beast.h"
#include "net.h"
#include <boost/smart_ptr.hpp>
#include <memory>
#include <string>

// Forward declaration
class shared_state;

class listener : public boost::enable_shared_from_this<listener>
{
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    boost::shared_ptr<shared_state> state_;

    void fail(beast::error_code ec, char const *what);
    void on_accept(beast::error_code ec, tcp::socket socket);

public:
    listener(net::io_context ioc, tcp::endpoint endpoint, boost::shared_ptr<shared_state> state);

    // Start accepting incoming requests
    void run();
};


#endif //BOOSTCHATSERVER_LISTENER_H

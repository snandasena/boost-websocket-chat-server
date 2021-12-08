//
// Created by sajith on 12/6/21.
//

#include "websocket_session.h"
#include <iostream>

websocket_session::websocket_session(tcp::socket &&socket, const boost::shared_ptr<shared_state> &state)
    : ws_(std::move(socket)), state_(state) {}

websocket_session::~websocket_session()
{
    state_->leave(this);
}


void websocket_session::fail(beast::error_code ec, const char *what)
{
    if (ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
    {
        std::cerr << what << ": " << ec.message() << '\n';
    }
}

void websocket_session::on_accept(beast::error_code ec)
{
    // handle error if any
    if (ec)
    {
        return fail(ec, "accept");
    }

    // Add this session to the list of active sessions
    state_->join(this);

    // Read message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void websocket_session::on_read(beast::error_code ec, std::size_t)
{
    if (ec)
    {
        return fail(ec, "read");
    }

    // send to all connections
    state_->send(beast::buffers_to_string(buffer_.data()));

    // read another message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}


void websocket_session::send(const boost::shared_ptr<const std::string> &ss)
{
    // post our work to the strand this ensures
    // that the members 'this' will not be
    // accessed concurrently

    net::post(
        ws_.get_executor(),
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this(),
            ss));
}

void websocket_session::on_send(const boost::shared_ptr<const std::string> &ss)
{

    //  always add to the queue
    queue_.push_back(ss);

    // are we already waiting??
    if (queue_.size() > 1)
    {
        return;
    }

    // we are not currently waiting, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        beast::bind_front_handler(
            &websocket_session::on_write,
            shared_from_this()));
}

void websocket_session::on_write(beast::error_code ec, std::size_t)
{
    if (ec)
    {
        return fail(ec, "write");
    }

    // remove the string from the queue
    queue_.erase(queue_.begin());

    // send the next message if any
    if (!queue_.empty())
    {
        ws_.async_write(
            net::buffer(*queue_.front()),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
    }
}
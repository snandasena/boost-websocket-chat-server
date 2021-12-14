//
// Created by sajith on 12/6/21.
//

#include "http_session.h"
#include "websocket_session.h"
#include <boost/config.hpp>
#include <iostream>

#define BOOST_NO_CXX14_GENERIC_LAMBDAS

beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind('.');
        if (pos == boost::string_view::npos)
        {
            return beast::string_view{};
        }
        return path.substr(pos);
    }();

    if (iequals(ext, ".htm")) return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php")) return "text/html";
    if (iequals(ext, ".css")) return "text/css";
    if (iequals(ext, ".txt")) return "text/plain";
    if (iequals(ext, ".js")) return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml")) return "application/xml";
    if (iequals(ext, ".swf")) return "application/x-shockwave-flash";
    if (iequals(ext, ".flv")) return "video/x-flv";
    if (iequals(ext, ".png")) return "image/png";
    if (iequals(ext, ".jpe")) return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg")) return "image/jpeg";
    if (iequals(ext, ".gif")) return "image/gif";
    if (iequals(ext, ".bmp")) return "image/bmp";
    if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif")) return "image/tiff";
    if (iequals(ext, ".svg")) return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

std::string path_cat(beast::string_view base, beast::string_view path)
{
    if (base.empty())
    {
        return std::string(path);
    }

    std::string result(base);


#ifdef BOOST_MSVC

    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
    {
        result.resize(result.size() - 1);
    }
    result.append(result.data(), path.size());
    for (auto &c: result)
    {
        if (c == '/')
        {
            c = path_separator;
        }
    }

#else

    if (char constexpr path_separator = '/';
        result.back() == path_separator)
    {
        result.resize(result.size() - 1);
    }
    result.append(result.data(), path.size());

#endif
    return result;
}


template<class Body, class Allocator, class Send>
void handle_request(beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send)
{

    // Returns a bad request response
    auto const bad_request = [&req](beast::string_view why)
    {
        http::response<http::string_body> res(http::status::bad_request, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found = [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    auto const server_error = [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred'" + std::string(why) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get &&
        req.method() != http::verb::head)
    {
        return send(bad_request("Unknown HTTP-method"));
    }

    // Request path must be absolute and not contain ".."
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
    {
        return send(bad_request("Illegal request-target"));
    }

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
    {
        path.append("index.html");
    }

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exiti
    if (ec == boost::system::errc::no_such_file_or_directory)
    {
        return send(not_found(req.target()));
    }

    // Handle an unknown error
    if (ec)
    {
        return send(server_error(ec.message()));
    }

    // Cache the size since we need it after the move
    auto const size = body.size();

    // response to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // response to GET request
    http::response<http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)),
                                        std::make_tuple(http::status::ok, req.version())};

    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(req));
}

struct http_session::send_lambda
{
    http_session &self_;

    explicit send_lambda(http_session &self) : self_(self)
    {

    }

    template<bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields> &&msg) const
    {
        auto sp = boost::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));
        auto self = self_.shared_from_this();
        http::async_write(
            self_.stream_,
            *sp,
            [self, sp](beast::error_code ec, std::size_t bytes)
            {
                self->on_write(ec, bytes, sp->need_eof());
            }
        );
    }
};

http_session::http_session(tcp::socket socket, boost::shared_ptr<shared_state> const &state)
    : stream_(
    std::move(socket)), state_(state) {}

void http_session::run()
{
    do_read();
}

void http_session::fail(beast::error_code ec, const char *what)
{
    // Don't report on canceled operations
    if (ec == net::error::operation_aborted)
    {
        return;
    }

    std::cerr << what << " : " << ec.message() << '\n';
}

void http_session::do_read()
{
    // Construct new parser for each message
    parser_.emplace();

    // Apply a reasonable limit to the allowed size
    // of the body in bytes to prevent abuse
    parser_->body_limit(10000);

    // set the timeout
    stream_.expires_after(std::chrono::seconds(30));

    //read a request
    http::async_read(stream_, buffer_, parser_->get(),
                     beast::bind_front_handler(&http_session::on_read,
                                               shared_from_this()));

}

void http_session::on_read(beast::error_code ec, std::size_t)
{

}

void http_session::on_write(beast::error_code ec, std::size_t, bool close)
{

}
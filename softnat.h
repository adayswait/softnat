#pragma once
#include <queue>
#include "asio.hpp"

template <typename T>
T &nullref() { return *static_cast<T *>(nullptr); }

constexpr const int MAX_LEN = 1024;

class twine : public std::enable_shared_from_this<twine>
{
public:
    //c2c
    twine(asio::io_context *io_context,
          asio::ip::tcp::endpoint &act_ep,
          asio::ip::tcp::endpoint &psv_ep)
        : _io_context(io_context),
          _act(*io_context),
          _psv(*io_context),
          _act_ep(act_ep),
          _psv_ep(psv_ep)
    {
    }
    //l2c
    twine(asio::io_context *io_context,
          asio::ip::tcp::socket act,
          asio::ip::tcp::endpoint &psv_ep)
        : _io_context(io_context),
          _act(std::move(act)),
          _psv(*io_context),
          _act_ep(nullref<asio::ip::tcp::endpoint>()),
          _psv_ep(psv_ep)
    {
    }
    //l2l
    twine(asio::io_context *io_context,
          asio::ip::tcp::socket act,
          asio::ip::tcp::socket psv)
        : _io_context(io_context),
          _act(std::move(act)),
          _psv(std::move(psv)),
          _act_ep(nullref<asio::ip::tcp::endpoint>()),
          _psv_ep(nullref<asio::ip::tcp::endpoint>())
    {
    }

    twine(const twine &) = delete;
    twine &operator=(const twine &) = delete;

    virtual void estab() = 0;

protected:
    void translate() {
        recv4act();
        recv4psv();
    }

    asio::io_context *_io_context;
    asio::ip::tcp::socket _act;       //active
    asio::ip::tcp::socket _psv;       //passive
    asio::ip::tcp::endpoint &_act_ep; //active endpoint
    asio::ip::tcp::endpoint &_psv_ep; //passive endpoint

private:
    void send2psv(std::size_t n) {
        auto self(shared_from_this());
        asio::async_write(_psv, asio::buffer(_act_buf, n),
            [this, self](const asio::error_code &ec, std::size_t n) {
                if (ec) return;
                recv4act();
        });
    }
    void send2act(std::size_t n) {
        auto self(shared_from_this());
        asio::async_write(_act, asio::buffer(_psv_buf, n),
            [this, self](const asio::error_code &ec, std::size_t n) {
                if (ec) return;
                recv4psv();
        });
    }
    void recv4psv() {
        auto self(shared_from_this());
        _psv.async_read_some(asio::buffer(_psv_buf, MAX_LEN),
            [this, self](const asio::error_code &ec, std::size_t n) {
                if (ec) return;
                send2act(n);
        });
    }
    void recv4act() {
        auto self(shared_from_this());
        _act.async_read_some(asio::buffer(_act_buf, MAX_LEN),
            [this, self](const asio::error_code &ec, std::size_t n) {
                if (ec) return;
                send2psv(n);
        });
    }
    char _act_buf[MAX_LEN];
    char _psv_buf[MAX_LEN];
};

//listen to connect
class l2c : public twine
{
public:
    using twine::twine;
    void estab() {
        auto self(shared_from_this());
        _psv.async_connect(_psv_ep, [this, self](const asio::error_code &ec) {
            if (ec) return;
            translate();
        });
    }
};

//listen to listen
class l2l : public twine
{
public:
    using twine::twine;
    void estab() {
        translate();
    }
};

//connect to connect
class c2c : public twine
{
public:
    using twine::twine;
    void estab() {
        auto self(shared_from_this());
        _act.async_connect(_act_ep,
            [this, self](const asio::error_code &ec) {
                if (ec) return;
                _psv.async_connect(_psv_ep,
                    [this, self](const asio::error_code &ec) {
                        if (ec) return;
                        translate();
                });
        });
    }
};
class smuggler
{
public:
    smuggler(const char *self_ip, unsigned short self_port,
             const char *dest_ip, unsigned short dest_port)
        : _io_context(std::thread::hardware_concurrency()),
          _self_ep(asio::ip::address_v4().from_string(self_ip), self_port),
          _dest_ep(asio::ip::address_v4().from_string(dest_ip), dest_port),
          _acceptor(_io_context, _self_ep)
    {
        _accept();
        _io_context.run();
    }

private:
    void _accept() {
        _acceptor.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket sock) {
                if (!ec) {
                    std::make_shared<l2c>(&_io_context,
                                          std::move(sock),
                                          _dest_ep)
                        ->estab();
                }
                _accept();
        });
    }
    asio::io_context _io_context;
    asio::ip::tcp::endpoint _self_ep;
    asio::ip::tcp::endpoint _dest_ep;
    asio::ip::tcp::acceptor _acceptor;
};

class handler
{
public:
    handler(const char *handler_ip,
            unsigned short insider_port, unsigned short handler_port)
        : _io_context(std::thread::hardware_concurrency()),
          _4insider_ep(asio::ip::address_v4().from_string(handler_ip),
                       insider_port),
          _4handler_ep(asio::ip::address_v4().from_string(handler_ip),
                       handler_port),
          _4insider(_io_context, _4insider_ep),
          _4handler(_io_context, _4handler_ep)
    {
        _wait_insider();
        _io_context.run();
    }

private:
    void _wait_insider() {
        _4insider.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket sock) {
                if (!ec) {
                    if (_cmd_sock == nullptr) {
                        _cmd_sock = std::make_shared<asio::ip::tcp::socket>(
                            std::move(sock));
                        _wait_handler();
                    } else {
                        if (_q4handler.empty()) {
                            _q4insider.push(std::move(sock));
                        } else {
                            std::make_shared<l2l>(&_io_context,
                                                std::move(_q4handler.front()),
                                                std::move(sock))
                                ->estab();
                            _q4handler.pop();
                        }
                    }
                }
                _wait_insider();
        });
    }
    void _wait_handler() {
        _4handler.async_accept(
            [this](std::error_code ec, asio::ip::tcp::socket sock) {
                if (!ec) {
                    if (_q4insider.empty()) {
                        _q4handler.push(std::move(sock));
                    } else {
                        std::make_shared<l2l>(&_io_context,
                                              std::move(sock),
                                              std::move(_q4insider.front()))
                            ->estab();
                        _q4insider.pop();
                    }
                    asio::async_write(*_cmd_sock, asio::buffer(_cmd_new, 1),
                        [this](const asio::error_code &ec, std::size_t n) {
                            if(ec) return;
                    });
                }
                _wait_handler();
        });
    }

    asio::io_context _io_context;
    asio::ip::tcp::endpoint _4insider_ep; //for date exchange
    asio::ip::tcp::endpoint _4handler_ep; //for operation
    asio::ip::tcp::acceptor _4insider;
    asio::ip::tcp::acceptor _4handler;
    std::queue<asio::ip::tcp::socket> _q4insider;
    std::queue<asio::ip::tcp::socket> _q4handler;
    std::shared_ptr<asio::ip::tcp::socket> _cmd_sock = nullptr;
    char _cmd_new[2];
};

class insider
{
public:
    insider(const char *insider_ip, unsigned short insider_port,
            const char *handler_ip, unsigned short handler_port)
        : _io_context(std::thread::hardware_concurrency()),
          _insider_ep(asio::ip::address_v4().from_string(insider_ip),
                      insider_port),
          _handler_ep(asio::ip::address_v4().from_string(handler_ip),
                      handler_port),
          _handler(_io_context)
    {
        _connect_handler();
        _io_context.run();
    }

private:
    void _connect_handler() {
        _handler.async_connect(_handler_ep,
            [this](const asio::error_code &ec) {
                if (ec) return;
                _recv_cmds();
        });
    }
    void _recv_cmds() {
        _handler.async_read_some(asio::buffer(_cmds, MAX_LEN),
            [this](const asio::error_code &ec, std::size_t n) {
                if (ec) return;
                for (int i = 0; i < n; i++) {
                    std::make_shared<c2c>(&_io_context,
                                        _handler_ep,
                                        _insider_ep)
                        ->estab();
                }
                _recv_cmds();
        });
    }

    asio::io_context _io_context;
    asio::ip::tcp::endpoint _insider_ep;
    asio::ip::tcp::endpoint _handler_ep;
    asio::ip::tcp::socket _handler;
    char _cmds[1024];
};
#pragma once

#include "asio.hpp"

constexpr const int MAX_LEN = 1024;

class twine : public std::enable_shared_from_this<twine>
{
public:
    twine(asio::ip::tcp::socket sock, asio::ip::tcp::socket oppo)
        : _sock(std::move(sock)), _oppo(std::move(oppo))
    {
    }
    twine(const twine &) = delete;
    twine &operator=(const twine &) = delete;
    void estab(asio::ip::tcp::endpoint sock_remote,
               asio::ip::tcp::endpoint oppo_remote)
    {
        estab_sock(sock_remote);
        estab_oppo(oppo_remote);
    }
    void translate() {
        recv_sock();
        recv_oppo();
    }
    void send_oppo(std::size_t recv_n) {
        auto self(shared_from_this());
        asio::async_write(_oppo, asio::buffer(_sockbuf, recv_n),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if (ec)
                    return;
                recv_sock();
        });
    }
    void send_sock(std::size_t recv_n) {
        auto self(shared_from_this());
        asio::async_write(_sock, asio::buffer(_oppobuf, recv_n),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if (ec)
                    return;
                recv_oppo();
        });
    }
    void recv_oppo() {
        auto self(shared_from_this());
        _oppo.async_read_some(asio::buffer(_oppobuf, MAX_LEN),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if (ec)
                    return;
                send_sock(recv_n);
        });
    }
    void recv_sock() {
        auto self(shared_from_this());
        _sock.async_read_some(asio::buffer(_sockbuf, MAX_LEN),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if (ec)
                    return;
                send_oppo(recv_n);
        });
    }
    void estab_oppo(asio::ip::tcp::endpoint remote) {
        if (_oppo.is_open()) {
            if (_sock.is_open()) {
                translate();
            }
        } else {
            auto self(shared_from_this());
            _oppo.async_connect(remote,
                [this, self](const asio::error_code &ec) {
                    if (ec) return;
                    if (_sock.is_open()) {
                        translate();
                    }
            });
        }
    }
    void estab_sock(asio::ip::tcp::endpoint remote) {
        if (_sock.is_open()) {
            if (_oppo.is_open()) {
                translate();
            }
        } else {
            auto self(shared_from_this());
            _sock.async_connect(remote,
                [this, self](const asio::error_code &ec) {
                    if (ec) return;
                    if (_oppo.is_open()){
                        translate();
                    }
            });
        }
    }

private:
    asio::ip::tcp::socket _sock; //source
    asio::ip::tcp::socket _oppo;
    char _sockbuf[MAX_LEN];
    char _oppobuf[MAX_LEN];
};

class undercover
{
public:
    undercover(asio::io_context *io_context,
               asio::ip::tcp::endpoint &insider,
               asio::ip::tcp::endpoint &handler)
        : _p_ioc(io_context),
          _ordersock(*io_context),
          _insider(insider),
          _handler(handler)
    {
    }

    void estab()
    {
        _ordersock.async_connect(_handler, 
            [this](const asio::error_code &ec) {
                if (ec)return;
                recv_order();
        });
    }

    void recv_order()
    {
        _ordersock.async_read_some(asio::buffer(_orders, MAX_LEN),
            [this](const asio::error_code &ec, std::size_t recv_n) {
                if (ec) return;
                for (size_t i = 0; i < recv_n; i++) {
                    create_twine();
                }
                recv_order();
        });
    }
    void create_twine()
    {
        std::make_shared<twine>(asio::ip::tcp::socket(*_p_ioc),
                                asio::ip::tcp::socket(*_p_ioc))
            ->estab(_handler, _insider);
    }

private:
    char _orders[MAX_LEN];
    asio::io_context *_p_ioc;
    asio::ip::tcp::socket _ordersock;
    asio::ip::tcp::endpoint &_handler;
    asio::ip::tcp::endpoint &_insider;
};

class handler
{
};
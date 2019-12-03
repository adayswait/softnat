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
    void translate()
    {
        recv_sock();
        recv_oppo();
    }
    void send_oppo(std::size_t recv_n)
    {
        auto self(shared_from_this());
        asio::async_write(_oppo, asio::buffer(_sockbuf, recv_n),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if(ec) return;
                recv_sock();
        });
    }
    void send_sock(std::size_t recv_n)
    {
        auto self(shared_from_this());
        asio::async_write(_sock, asio::buffer(_oppobuf, recv_n), 
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if(ec) return;
                recv_oppo();
        });
    }
    void recv_oppo()
    {
        auto self(shared_from_this());
        _oppo.async_read_some(asio::buffer(_oppobuf, MAX_LEN),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if(ec) return;
                send_sock(recv_n);
        });
    };
    void recv_sock()
    {
        auto self(shared_from_this());
        _sock.async_read_some(asio::buffer(_sockbuf, MAX_LEN),
            [this, self](const asio::error_code &ec, std::size_t recv_n) {
                if(ec) return;
                send_oppo(recv_n);
        });
    };
    void dial_oppo(asio::ip::tcp::endpoint remote)
    {
        auto self(shared_from_this());
        _oppo.async_connect(remote,
            [this, self](const asio::error_code &ec) {
                if(ec) return;
                asio::ip::tcp::no_delay option(true);
                translate();
        });
    }

private:
    asio::ip::tcp::socket _sock; //source
    asio::ip::tcp::socket _oppo;
    char _sockbuf[MAX_LEN];
    char _oppobuf[MAX_LEN];
};
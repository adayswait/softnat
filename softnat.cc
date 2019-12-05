#include <iostream>
#include <thread>
#include "softnat.h"

using tcp_t = asio::ip::tcp;
using addr_t = asio::ip::address_v4;

void l2c(const char *ip, unsigned short port,
         const char *dest_ip, unsigned short dest_port)
{
    asio::io_context io_context(std::thread::hardware_concurrency());
    tcp_t::endpoint localhost(addr_t().from_string(ip), port);
    tcp_t::endpoint remote(addr_t().from_string(dest_ip), dest_port);
    tcp_t::acceptor acceptor(io_context, std::move(localhost));
    std::function<void()> ac = [&]() -> void {
        acceptor.async_accept([&](std::error_code ec, tcp_t::socket sock) {
            if (!ec)
            {
                auto oppo = tcp_t::socket(io_context);
                std::make_shared<twine>(std::move(sock), std::move(oppo))
                    ->estab(localhost, remote);
            }
            ac();
        });
    };
    ac();
    io_context.run();
}

void c2c(const char *insider_ip, unsigned short insider_port,
         const char *handler_ip, unsigned short handler_port)
{
    asio::io_context io_context(std::thread::hardware_concurrency());
    tcp_t::endpoint insider(addr_t().from_string(insider_ip), insider_port);
    tcp_t::endpoint handler(addr_t().from_string(handler_ip), handler_port);
    undercover uc(&io_context, insider, handler);
    uc.estab();
    io_context.run();
}

int main(int argc, char *argv[])
{
    // l2c("10.1.1.248", 3001, "10.1.8.135", 3000);
    c2c("10.1.8.135", 3002, "10.1.8.135", 3001);
    return 0;
}

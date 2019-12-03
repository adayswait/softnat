#include <iostream>
#include "softnat.h"

using tcp_t = asio::ip::tcp;
using addr_t = asio::ip::address_v4;

void l2c(const char *ip, unsigned short port,
         const char *dest_ip, unsigned short dest_port)
{
    asio::io_context io_context;
    tcp_t::endpoint localhost(addr_t().from_string(ip), port);
    tcp_t::endpoint remote(addr_t().from_string(dest_ip), dest_port);
    tcp_t::acceptor acceptor(io_context, std::move(localhost));
    std::function<void()> ac =[&]() -> void {
        acceptor.async_accept([&](std::error_code ec, tcp_t::socket sock) {
            if (!ec)
            {
                auto oppo = tcp_t::socket(io_context);
                std::make_shared<twine>(std::move(sock), std::move(oppo))
                    ->dial_oppo(remote);
            }
            ac();
        });
    };
    ac();
    io_context.run();
}

int main(int argc, char *argv[])
{
    // l2c("10.1.1.248", 3001, "10.1.8.135", 3000);
    l2c("10.1.1.248", 3001, "10.1.1.248", 6379);
    return 0;
}
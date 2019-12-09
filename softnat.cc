#include <queue>
#include "softnat.h"

namespace softnat
{

twine::twine(asio::io_context *io_context,
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
twine::twine(asio::io_context *io_context,
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
twine::twine(asio::io_context *io_context,
             asio::ip::tcp::socket act,
             asio::ip::tcp::socket psv)
    : _io_context(io_context),
      _act(std::move(act)),
      _psv(std::move(psv)),
      _act_ep(nullref<asio::ip::tcp::endpoint>()),
      _psv_ep(nullref<asio::ip::tcp::endpoint>())
{
}

void twine::translate() {
    recv4act();
    recv4psv();
}

void twine::send2psv(std::size_t n) {
    auto self(shared_from_this());
    asio::async_write(_psv, asio::buffer(_act_buf, n),
        [this, self](const asio::error_code &ec, std::size_t n) {
            if (ec) return;
            recv4act();
    });
}
void twine::send2act(std::size_t n) {
    auto self(shared_from_this());
    asio::async_write(_act, asio::buffer(_psv_buf, n),
        [this, self](const asio::error_code &ec, std::size_t n) {
            if (ec) return;
            recv4psv();
    });
}
void twine::recv4psv() {
    auto self(shared_from_this());
    _psv.async_read_some(asio::buffer(_psv_buf, MAX_LEN),
        [this, self](const asio::error_code &ec, std::size_t n) {
            if (ec) return;
            send2act(n);
    });
}
void twine::recv4act() {
    auto self(shared_from_this());
    _act.async_read_some(asio::buffer(_act_buf, MAX_LEN),
        [this, self](const asio::error_code &ec, std::size_t n) {
            if (ec) return;
            send2psv(n);
    });
}

void l2c::estab() {
    auto self(shared_from_this());
    _psv.async_connect(_psv_ep, [this, self](const asio::error_code &ec) {
        if (ec) return;
        translate();
    });
}

void l2l::estab() {
    translate();
}

//connect to connect
void c2c::estab() {
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

smuggler::smuggler(const char *self_ip, unsigned short self_port,
                   const char *dest_ip, unsigned short dest_port)
    : _io_context(std::thread::hardware_concurrency()),
      _self_ep(asio::ip::address_v4().from_string(self_ip), self_port),
      _dest_ep(asio::ip::address_v4().from_string(dest_ip), dest_port),
      _acceptor(_io_context, _self_ep)
{
    _accept();
    _io_context.run();
}

void smuggler::_accept() {
    _acceptor.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket sock) {
            if (!ec)
            {
                std::make_shared<l2c>(&_io_context,
                                      std::move(sock),
                                      _dest_ep)
                    ->estab();
            }
            _accept();
    });
}

handler::handler(const char *handler_ip,
                 unsigned short handler_port, unsigned short insider_port)
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

void handler::_wait_insider() {
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
void handler::_wait_handler() {
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
                        if (ec) return;
                });
            }
            _wait_handler();
    });
}

insider::insider(const char *handler_ip, unsigned short handler_port,
                 const char *insider_ip, unsigned short insider_port)
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

void insider::_connect_handler() {
    _handler.async_connect(_handler_ep,
        [this](const asio::error_code &ec) {
            if (ec) return;
            _recv_cmds();
    });
}
void insider::_recv_cmds() {
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

} // namespace softnat
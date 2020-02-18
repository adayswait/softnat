#pragma once
#include <queue>
#include "asio.hpp"

namespace softnat
{

template <typename T>
T &nullref() { return *static_cast<T *>(nullptr); }

constexpr const int MAX_LEN = 1024;

class twine : public std::enable_shared_from_this<twine>
{
public:
    //c2c
    twine(asio::io_context *io_context,
          asio::ip::tcp::endpoint &act_ep,
          asio::ip::tcp::endpoint &psv_ep);
    //l2c
    twine(asio::io_context *io_context,
          asio::ip::tcp::socket act,
          asio::ip::tcp::endpoint &psv_ep);
    //l2l
    twine(asio::io_context *io_context,
          asio::ip::tcp::socket act,
          asio::ip::tcp::socket psv);

    twine(const twine &) = delete;
    twine &operator=(const twine &) = delete;

    virtual void estab() = 0;

protected:
    void translate();
    asio::io_context *_io_context;
    asio::ip::tcp::socket _act;       //active
    asio::ip::tcp::socket _psv;       //passive
    asio::ip::tcp::endpoint &_act_ep; //active endpoint
    asio::ip::tcp::endpoint &_psv_ep; //passive endpoint

private:
    void send2psv(std::size_t n);
    void send2act(std::size_t n);
    void recv4psv();
    void recv4act();
    void close();
    char _act_buf[MAX_LEN];
    char _psv_buf[MAX_LEN];
};

//listen to connect
class l2c : public twine
{
public:
    using twine::twine;
    void estab();
};

//listen to listen
class l2l : public twine
{
public:
    using twine::twine;
    void estab();
};

//connect to connect
class c2c : public twine
{
public:
    using twine::twine;
    void estab();
};
class smuggler
{
public:
    smuggler(const char *self_ip, unsigned short self_port,
             const char *dest_ip, unsigned short dest_port);

private:
    void _accept();
    asio::io_context _io_context;
    asio::ip::tcp::endpoint _self_ep;
    asio::ip::tcp::endpoint _dest_ep;
    asio::ip::tcp::acceptor _acceptor;
};

class handler
{
public:
    handler(const char *handler_ip,
            unsigned short handler_port, unsigned short insider_port);

private:
    void _wait_insider();
    void _wait_handler();
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
    insider(const char *handler_ip, unsigned short handler_port,
            const char *insider_ip, unsigned short insider_port);

private:
    void _connect_handler();
    void _recv_cmds();

    asio::io_context _io_context;
    asio::ip::tcp::endpoint _insider_ep;
    asio::ip::tcp::endpoint _handler_ep;
    asio::ip::tcp::socket _handler;
    char _cmds[MAX_LEN];
};

} // namespace softnat
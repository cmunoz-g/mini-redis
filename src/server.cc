#include "server.hh"
#include "socket.hh"
#include "kv.hh"
#include <poll.h>
#include <cstring>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>

static constexpr size_t BUFFER_SIZE_KB = 64;
static constexpr size_t BYTES_IN_KB = 1024;

struct Conn {
    int fd{-1};
    bool want_read{true}, want_write{false}, want_close{false};
    Buffer in, out;
};

static void do_request(HMap &db, std::vector<std::string> &cmd, Buffer &out) { 
    if (cmd.size() == 2 && cmd[0] == "get") {
        return do_get(db, cmd, out);  
    }
    else if (cmd.size() == 3 && cmd[0] == "set") {
        return do_set(db, cmd, out);
    }
    else if (cmd.size() == 2 && cmd[0] == "del") {
        do_del(db, cmd, out);
    }
    // else {
    //     out.status = RES_ERR;
    // }
}

static bool handle_request(HMap &db, Conn *conn) {
    size_t size = conn->in.data_end - conn->in.data_begin;
    if (size < 4) return false;

    uint32_t len = 0;
    memcpy(&len, conn->in.data_begin, 4);
    if (len > MSG_SIZE_LIMIT) {
        conn->want_close = true;
        return false;
    }

    if (4 + len > size) return false;

    const uint8_t *request = conn->in.data_begin + 4;
    
    std::vector<std::string> cmd;
    if (parse_request(request, len, cmd) > 0) {
        conn->want_close = true;
        return false;
    }

    size_t header_pos = 0;
    response_begin(conn->out, &header_pos);
    do_request(db, cmd, conn->out);
    response_end(conn->out, header_pos);

    return true;
}

static void handle_write(Conn *conn) {
    size_t size = conn->out.data_end - conn->out.data_begin;

    assert(size > 0);
    ssize_t rv = ::write(conn->fd, conn->out.data_begin, size);
    if (rv < 0) {
        if (errno == EAGAIN) return;
        conn->want_close = true;
        return;
    }

    buf_consume(conn->out, static_cast<size_t>(rv));
    if (conn->out.data_begin == conn->out.data_end) {
        conn->want_read = true;
        conn->want_write = false;
    }
}

static void handle_read(HMap &db, Conn *conn) {
    uint8_t buf[BUFFER_SIZE_KB * BYTES_IN_KB];
    ssize_t rv = ::read(conn->fd, buf, sizeof(buf));
    if (rv <= 0) {
        conn->want_close = true;
        return;
    }
    buf_append(conn->in, buf, static_cast<size_t>(rv));
    while (handle_request(db, conn)) {};
    if (conn->out.data_begin != conn->out.data_end) {
        conn->want_read = false;
        conn->want_write = true;
        return handle_write(conn);
    }
}

static Conn *handle_accept(int fd) { 
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = ::accept(fd, reinterpret_cast<sockaddr *>(&client_addr), &addrlen);
    if (connfd < 0) return nullptr;
    if (set_non_blocking(connfd)) (void)0; // handle error
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;

    constexpr size_t buffer_size = BUFFER_SIZE_KB * BYTES_IN_KB;
    uint8_t *inbuf = new uint8_t[buffer_size];
    uint8_t *outbuf = new uint8_t[buffer_size];

    conn->in = {inbuf, inbuf + buffer_size, inbuf, inbuf};
    conn->out = {outbuf, outbuf + buffer_size, outbuf, outbuf};

    return conn;
}


int run_server(HMap &db, const char* host, uint16_t port) {
    int server_fd = socket_listen(host, port);
    if (server_fd < 0) return 1; // think through error mgmt

    std::vector<Conn *> fd2conn;
    std::vector<struct pollfd> pfds;
    
    for (;;) {
        pfds.clear();
        pfds.push_back(pollfd{server_fd, POLLIN, 0});

        for (int fd = 0; fd < (int)pfds.size(); ++fd) {
            Conn *c = fd2conn[fd];
            if (!c) continue;
            struct pollfd pfd = {c->fd, POLLERR, 0};
            if (c->want_read) pfd.events |= POLLIN;
            if (c->want_write) pfd.events |= POLLOUT;
            pfds.push_back(pfd);
        }
        
        int rv = ::poll(pfds.data(), (nfds_t)pfds.size(), -1);
        if (rv < 0) {
            if (errno == EINTR) continue;
            //else manage fatal error 
        }

        if (pfds[0].revents & POLLIN) { // revents in the server_fd aka new connections
            if (Conn *c = handle_accept(server_fd)) {
                if (fd2conn.size() <= (size_t)c->fd) fd2conn.resize(c->fd + 1);
                fd2conn[c->fd] = c;
            }
        }

        for (size_t i = 1; i < pfds.size(); ++i) {
            uint32_t re = pfds[i].revents;
            Conn *c = fd2conn[pfds[i].fd];
            if (re & POLLIN) handle_read(db, c);
            if (re & POLLOUT) handle_write(c);
            if (re & POLLERR || c->want_close) {
                (void)close(c->fd);
                fd2conn[c->fd] = nullptr; // is this correct ?
                delete c;
            }
        }
    } 

    return (0);
}
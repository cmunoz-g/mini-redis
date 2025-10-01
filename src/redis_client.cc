
// int redis_client(uint16_t port) { // test function ?
//     int fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (fd < 0) return 1;

//     struct sockaddr_in addr = {};
//     addr.sin_family = AF_INET;
//     addr.sin_port = ntohs(port);
//     addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
//     int rv = ::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
//     if (rv) return 1;
    
//     char msg[] = "hello";
//     write(fd, msg, std::strlen(msg));

//     char rbuf[64] = {};
//     ssize_t nbytes = read(fd, rbuf, sizeof(rbuf) - 1);
//     if (nbytes < 0) return 1;

//     std::printf("server says : %s\n", rbuf);
//     close(fd);
//     return 0;
// }

// int main() {
//     return (redis_client(1234));
// }
/*
 * Regents of the Univeristy of California, All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef MVVM_WAMR_READ_WRITE_H
#define MVVM_WAMR_READ_WRITE_H
#include <ylt/struct_pack.hpp>
#include <cstdint>
#ifndef BH_PLATFORM_WINDOWS
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

struct WriteStream {
    virtual bool write(const char *data, std::size_t sz) const
    {
        return false;
    };
    virtual ~WriteStream() = default;
};
struct ReadStream {
    virtual bool read(char *data, std::size_t sz) const { return false; };
    virtual bool ignore(std::size_t sz) const { return false; };
    virtual const char *read_view(size_t len) { return nullptr; }
    virtual std::size_t tellg() const { return 0; };
    virtual ~ReadStream() = default;
};
struct FwriteStream : public WriteStream {
    FILE *file;
    bool write(const char *data, std::size_t sz) const override
    {
        return fwrite(data, sz, 1, file) == 1;
    }
    explicit FwriteStream(const char *file_name)
      : file(fopen(file_name, "wb"))
    {}
    ~FwriteStream() override { fclose(file); }
};
struct FreadStream : public ReadStream {
    FILE *file;
    bool read(char *data, std::size_t sz) const override
    {
        return fread(data, sz, 1, file) == 1;
    }
    const char *read_view(size_t len) override
    {
        char *buffer = new char[len];
        if (fread(buffer, len, 1, file) != 1) {
            delete[] buffer;
            return nullptr;
        }
        return buffer;
    }
    bool ignore(std::size_t sz) const override
    {
        return fseek(file, sz, SEEK_CUR) == 0;
    }
    std::size_t tellg() const override { return ftell(file); }
    explicit FreadStream(const char *file_name)
      : file(fopen(file_name, "rb"))
    {}
    ~FreadStream() override { fclose(file); }
};
#ifndef BH_PLATFORM_WINDOWS
struct SocketWriteStream : public WriteStream {
    int sock_fd;
    bool write(const char *data, std::size_t sz) const override
    {
        std::size_t totalSent = 0;
        while (totalSent < sz) {
            ssize_t sent = send(sock_fd, data + totalSent, sz - totalSent, 0);
            if (sent == -1) {
                return false;
            }
            totalSent += sent;
        }
        return true;
    }
    explicit SocketWriteStream(const char *address, int port)
    {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            LOG_DEBUG("Socket creation failed");
            return;
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, address, &server_addr.sin_addr);
        if (connect(sock_fd, (struct sockaddr *)&server_addr,
                    sizeof(server_addr))
            == -1) {
            LOG_DEBUG("Connection failed");
            close(sock_fd);
            exit(EXIT_FAILURE);
        }
    }
    ~SocketWriteStream() override { close(sock_fd); }
};
struct SocketReadStream : public ReadStream {
    int sock_fd;
    int client_fd;
    mutable std::size_t position = 0;
    bool read(char *data, std::size_t sz) const override
    {
        std::size_t totalReceived = 0;
        while (totalReceived < sz) {
            ssize_t received =
                recv(client_fd, data + totalReceived, sz - totalReceived, 0);
            if (received == -1) {
                return false;
            }
            else if (received == 0) {
                return false;
            }
            totalReceived += received;
        }
        position += totalReceived;
        return true;
    }
    const char *read_view(size_t len) override
    {
        char *buffer = new char[len];
        std::size_t totalReceived = 0;
        while (totalReceived < len) {
            ssize_t received =
                recv(client_fd, buffer + totalReceived, len - totalReceived, 0);
            if (received == -1 || received == 0) {
                return nullptr;
            }
            totalReceived += received;
        }
        position += len;

        return buffer;
    }
    explicit SocketReadStream(const char *address, int port)
    {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            LOG_DEBUG("Socket creation failed");
            return;
        }
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, address, &server_addr.sin_addr);
        auto addr_len = sizeof(server_addr);
        LOG_DEBUG("[Server] Bind socket %d %d", address, port);
        if (bind(sock_fd, (struct sockaddr *)&server_addr, addr_len) < 0) {
            LOG_DEBUG("Bind failed%d", strerror(errno));
            exit(EXIT_FAILURE);
        }
        LOG_DEBUG("[Server] Listening on socket");
        if (listen(sock_fd, 3) < 0) {
            LOG_DEBUG("Listen failed");
            exit(EXIT_FAILURE);
        }
        client_fd = accept(sock_fd, (struct sockaddr *)&server_addr,
                           (socklen_t *)&addr_len);
    }
    bool ignore(std::size_t sz) const override
    {
        char buffer[1024];
        std::size_t total_ignored = 0;
        while (total_ignored < sz) {
            std::size_t to_ignore =
                std::min(sz - total_ignored, sizeof(buffer));
            ssize_t ignored = recv(client_fd, buffer, to_ignore, 0);
            if (ignored <= 0) {
                return false;
            }
            total_ignored += ignored;
            position += ignored;
        }
        return true;
    }
    std::size_t tellg() const override { return position; }
    ~SocketReadStream() override { close(sock_fd); }
};
#endif

#endif /* MVVM_WAMR_READ_WRITE_H */

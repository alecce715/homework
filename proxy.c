#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 8192
#define SOCKS4_VERSION 4
#define SOCKS5_VERSION 5

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

unsigned short port = 1080;

void log_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[%s] ", __TIME__);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

int connect_to_target(const char *host, unsigned short target_port) {
    struct addrinfo hints, *result, *rp;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[6];
    sprintf(port_str, "%d", target_port);

    int rv = getaddrinfo(host, port_str, &hints, &result);
    if (rv != 0) {
        log_message("getaddrinfo failed: %s", gai_strerror(rv));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == INVALID_SOCKET) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, (int)rp->ai_addrlen) != SOCKET_ERROR) {
            freeaddrinfo(result);
            return sockfd;
        }

        closesocket(sockfd);
    }

    freeaddrinfo(result);
    log_message("Failed to connect to target: %s:%d", host, target_port);
    return -1;
}

void socks4_response(SOCKET client_fd, unsigned char status, unsigned int bind_ip, unsigned short bind_port) {
    unsigned char response[8];
    response[0] = 0;
    response[1] = status;
    response[2] = (bind_port >> 8) & 0xFF;
    response[3] = bind_port & 0xFF;
    response[4] = (bind_ip >> 24) & 0xFF;
    response[5] = (bind_ip >> 16) & 0xFF;
    response[6] = (bind_ip >> 8) & 0xFF;
    response[7] = bind_ip & 0xFF;
    send(client_fd, (char *)response, 8, 0);
}

int handle_socks4(SOCKET client_fd) {
    unsigned char buffer[BUFFER_SIZE];
    int n = recv(client_fd, (char *)buffer, BUFFER_SIZE, 0);
    if (n <= 0) {
        log_message("SOCKS4: Failed to read request");
        return -1;
    }

    if (buffer[0] != SOCKS4_VERSION) {
        log_message("SOCKS4: Invalid version");
        socks4_response(client_fd, 91, 0, 0);
        return -1;
    }

    unsigned char cmd = buffer[1];
    if (cmd != 1) {
        log_message("SOCKS4: Unsupported command %d", cmd);
        socks4_response(client_fd, 91, 0, 0);
        return -1;
    }

    unsigned short target_port = (buffer[2] << 8) | buffer[3];
    unsigned int target_ip = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];

    char host[256];
    int is_socks4a = (buffer[4] == 0 && buffer[5] == 0 && buffer[6] == 0 && buffer[7] != 0);

    if (is_socks4a) {
        int idx = 8;
        while (idx < n && buffer[idx] != 0) idx++;
        idx++;
        int host_len = 0;
        while (idx < n && buffer[idx] != 0) {
            host[host_len++] = buffer[idx++];
        }
        host[host_len] = '\0';
        log_message("SOCKS4a: Connecting to %s:%d", host, target_port);
    } else {
        sprintf(host, "%d.%d.%d.%d", 
            (target_ip >> 24) & 0xFF,
            (target_ip >> 16) & 0xFF,
            (target_ip >> 8) & 0xFF,
            target_ip & 0xFF);
        log_message("SOCKS4: Connecting to %s:%d", host, target_port);
    }

    SOCKET target_fd = connect_to_target(host, target_port);
    if (target_fd == -1) {
        socks4_response(client_fd, 91, 0, 0);
        return -1;
    }

    SOCKADDR_IN local_addr;
    int addr_len = sizeof(local_addr);
    getsockname(target_fd, (SOCKADDR *)&local_addr, &addr_len);

    socks4_response(client_fd, 90, ntohl(local_addr.sin_addr.s_addr), ntohs(local_addr.sin_port));
    log_message("SOCKS4: Connection established");

    return target_fd;
}

void socks5_response(SOCKET client_fd, unsigned char status, const char *bind_addr, unsigned short bind_port) {
    unsigned char response[BUFFER_SIZE];
    int len = 0;

    response[len++] = SOCKS5_VERSION;
    response[len++] = status;
    response[len++] = 0;

    struct in_addr addr;
    if (inet_pton(AF_INET, bind_addr, &addr) == 1) {
        response[len++] = 1;
        memcpy(&response[len], &addr.s_addr, 4);
        len += 4;
    } else {
        response[len++] = 3;
        response[len++] = (unsigned char)strlen(bind_addr);
        memcpy(&response[len], bind_addr, strlen(bind_addr));
        len += strlen(bind_addr);
    }

    response[len++] = (bind_port >> 8) & 0xFF;
    response[len++] = bind_port & 0xFF;

    send(client_fd, (char *)response, len, 0);
}

int handle_socks5_auth(SOCKET client_fd) {
    unsigned char buffer[BUFFER_SIZE];
    int n = recv(client_fd, (char *)buffer, BUFFER_SIZE, 0);
    if (n < 2) {
        log_message("SOCKS5: Failed to read auth request");
        return -1;
    }

    if (buffer[0] != SOCKS5_VERSION) {
        log_message("SOCKS5: Invalid version");
        return -1;
    }

    unsigned char methods_count = buffer[1];
    int i;
    for (i = 0; i < methods_count && i + 2 < n; i++) {
        if (buffer[2 + i] == 0) {
            unsigned char response[2] = {SOCKS5_VERSION, 0};
            send(client_fd, (char *)response, 2, 0);
            return 0;
        }
    }

    unsigned char response[2] = {SOCKS5_VERSION, 0xFF};
    send(client_fd, (char *)response, 2, 0);
    log_message("SOCKS5: No acceptable auth method");
    return -1;
}

int handle_socks5(SOCKET client_fd) {
    if (handle_socks5_auth(client_fd) != 0) {
        return -1;
    }

    unsigned char buffer[BUFFER_SIZE];
    int n = recv(client_fd, (char *)buffer, BUFFER_SIZE, 0);
    if (n < 10) {
        log_message("SOCKS5: Failed to read request");
        return -1;
    }

    if (buffer[0] != SOCKS5_VERSION) {
        log_message("SOCKS5: Invalid version in request");
        socks5_response(client_fd, 1, "0.0.0.0", 0);
        return -1;
    }

    unsigned char cmd = buffer[1];
    if (cmd != 1) {
        log_message("SOCKS5: Unsupported command %d", cmd);
        socks5_response(client_fd, 7, "0.0.0.0", 0);
        return -1;
    }

    unsigned char addr_type = buffer[3];
    char host[256];
    unsigned short target_port;
    int idx = 4;

    if (addr_type == 1) {
        if (n < idx + 6) {
            log_message("SOCKS5: Invalid IPv4 address");
            socks5_response(client_fd, 1, "0.0.0.0", 0);
            return -1;
        }
        sprintf(host, "%d.%d.%d.%d", buffer[idx], buffer[idx+1], buffer[idx+2], buffer[idx+3]);
        idx += 4;
        target_port = (buffer[idx] << 8) | buffer[idx+1];
        log_message("SOCKS5: Connecting to IPv4 %s:%d", host, target_port);
    } else if (addr_type == 3) {
        if (n < idx + 1) {
            log_message("SOCKS5: Invalid domain name");
            socks5_response(client_fd, 1, "0.0.0.0", 0);
            return -1;
        }
        unsigned char host_len = buffer[idx++];
        if (n < idx + host_len + 2) {
            log_message("SOCKS5: Invalid domain name length");
            socks5_response(client_fd, 1, "0.0.0.0", 0);
            return -1;
        }
        memcpy(host, &buffer[idx], host_len);
        host[host_len] = '\0';
        idx += host_len;
        target_port = (buffer[idx] << 8) | buffer[idx+1];
        log_message("SOCKS5: Connecting to domain %s:%d", host, target_port);
    } else if (addr_type == 4) {
        log_message("SOCKS5: IPv6 not supported");
        socks5_response(client_fd, 1, "0.0.0.0", 0);
        return -1;
    } else {
        log_message("SOCKS5: Unknown address type %d", addr_type);
        socks5_response(client_fd, 1, "0.0.0.0", 0);
        return -1;
    }

    SOCKET target_fd = connect_to_target(host, target_port);
    if (target_fd == -1) {
        socks5_response(client_fd, 1, "0.0.0.0", 0);
        return -1;
    }

    SOCKADDR_IN local_addr;
    int addr_len = sizeof(local_addr);
    getsockname(target_fd, (SOCKADDR *)&local_addr, &addr_len);

    char bind_addr[16];
    inet_ntop(AF_INET, &local_addr.sin_addr, bind_addr, sizeof(bind_addr));
    socks5_response(client_fd, 0, bind_addr, ntohs(local_addr.sin_port));
    log_message("SOCKS5: Connection established");

    return target_fd;
}

void proxy_data(SOCKET client_fd, SOCKET target_fd) {
    fd_set read_fds;
    int max_fd = (client_fd > target_fd) ? client_fd : target_fd;
    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        FD_SET(target_fd, &read_fds);

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret == SOCKET_ERROR) {
            log_message("select error");
            break;
        }

        if (FD_ISSET(client_fd, &read_fds)) {
            int n = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (n <= 0) break;
            send(target_fd, buffer, n, 0);
        }

        if (FD_ISSET(target_fd, &read_fds)) {
            int n = recv(target_fd, buffer, BUFFER_SIZE, 0);
            if (n <= 0) break;
            send(client_fd, buffer, n, 0);
        }
    }
}

unsigned __stdcall client_handler(void *param) {
    SOCKET client_fd = (SOCKET)param;
    unsigned char version;

    int ret = recv(client_fd, (char *)&version, 1, MSG_PEEK);
    if (ret != 1) {
        log_message("Failed to read protocol version");
        closesocket(client_fd);
        _endthreadex(0);
        return 0;
    }

    SOCKET target_fd = -1;

    if (version == SOCKS4_VERSION) {
        target_fd = handle_socks4(client_fd);
    } else if (version == SOCKS5_VERSION) {
        target_fd = handle_socks5(client_fd);
    } else {
        log_message("Unknown protocol version %d", version);
        closesocket(client_fd);
        _endthreadex(0);
        return 0;
    }

    if (target_fd != -1) {
        proxy_data(client_fd, target_fd);
        closesocket(target_fd);
    }

    closesocket(client_fd);
    log_message("Connection closed");
    _endthreadex(0);
    return 0;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            port = (unsigned short)atoi(argv[i + 1]);
            i++;
        }
    }

    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        printf("WSAStartup failed: %d\n", ret);
        return 1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    SOCKADDR_IN server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (SOCKADDR *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 100) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("SOCKS4/SOCKS5 proxy server listening on port %d\n", port);
    fflush(stdout);

    while (1) {
        SOCKADDR_IN client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (SOCKADDR *)&client_addr, &addr_len);

        if (client_fd == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            continue;
        }

        char client_ip[16];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        log_message("New connection from %s:%d", client_ip, ntohs(client_addr.sin_port));

        unsigned int thread_id;
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, client_handler, (void *)client_fd, 0, &thread_id);
        if (thread == NULL) {
            log_message("Failed to create thread");
            closesocket(client_fd);
        } else {
            CloseHandle(thread);
        }
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
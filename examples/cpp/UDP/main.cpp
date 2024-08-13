#include <iostream>
#include <cstring> 
#include <cstdlib> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#define GROUP "224.0.2.3" 
#define PORT 51213
#define LOCALIP "192.168.195.3"

int main() {
    int sock;
    struct sockaddr_in server_address;
    struct ip_mreq group;

    // 创建套接字
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Could not create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // server_address.sin_addr.s_addr = inet_addr(LOCALIP);

    // 绑定套接字
    if (bind(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 设置组播选项
    group.imr_multiaddr.s_addr = inet_addr(GROUP);
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    group.imr_interface.s_addr = server_address.sin_addr.s_addr;
    // server_address.sin_addr.s_addr = inet_addr(LOCALIP);


    // 加入组播组
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group)) < 0) {
        std::cerr << "Could not join group: " << strerror(errno) << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }

    std::cout << "Receiving messages on port " << PORT << " from group " << GROUP << std::endl;

    // 接收数据
    char buffer[1500];
    struct sockaddr_in sender_address;
    socklen_t sender_address_len = sizeof(sender_address);
    while (true) {
        ssize_t bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0,
                                          (struct sockaddr *)&sender_address, &sender_address_len);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // 确保字符串以空字符结尾
            std::cout << "Received message: " << bytes_received << std::endl;
            for (int i = 14; i < 28 ; i++){
                        printf(" %x",buffer[i]);
                // std::cout << (buffer[i]).str() << " ";
            }
            std::cout << std::endl;
            // std::this_thread::sleep_for(std::chrono::seconds(1));

            // break;
        } else if (bytes_received < 0) {
            std::cerr << "Recvfrom error" << std::endl;
            break;
        }
    }

    close(sock);
    return 0;
}
#include <iostream>
#include <cstring> 
#include <cstdlib> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "UdpReciver.hpp"
// #include "CLIParser.hpp"

namespace eprosima {
namespace fastdds {
namespace examples {
namespace configuration {

// UdpReciver::UdpReciver(const char* groupIP, uint16_t port, uint16_t bufferlen)
UdpReciver::UdpReciver(std::string groupIP, const uint16_t port)

{
  int sock_;
  sockaddr_in server_address;
  ip_mreq group;

  // bufferlen_ = bufferlen;
  // buffer_ = new char[bufferlen + 1];

  sockaddr_in sender_address_;
  socklen_t sender_address_len = sizeof(sender_address_);
  
  if ((sock_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      std::cerr << "Could not create socket" << std::endl;
      exit(EXIT_FAILURE);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock_, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
      std::cerr << "Bind failed" << std::endl;
      close(sock_);
      exit(EXIT_FAILURE);
  }

  group.imr_multiaddr.s_addr = inet_addr(groupIP.c_str());
  group.imr_interface.s_addr = htonl(INADDR_ANY);
  group.imr_interface.s_addr = server_address.sin_addr.s_addr;

  if (setsockopt(sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group)) < 0) {
      std::cerr << "Could not join group: " << strerror(errno) << std::endl;
      close(sock_);
      exit(EXIT_FAILURE);
  }

  std::cout << "Creat UDPreciver success" << std::endl;

}
UdpReciver::~UdpReciver()
{

  close(sock_);
  // delete[] buffer_;

}

long UdpReciver::run(char** buffer)
{
  long bytes_received = recvfrom(sock_, *buffer, sizeof(*buffer), 0,
                                    (struct sockaddr *)&sender_address_, &sender_address_len_);
  if (bytes_received > 0) {
      *buffer[bytes_received] = '\0'; // 确保字符串以空字符结尾
      // std::cout << "Received message: " << bytes_received << std::endl;
      // for (int i = 0; i < bytes_received ; i++){
      //     std::cout << int(buffer[i]) << " ";
      // }
      // std::cout << std::endl;
      // break;
      return bytes_received;

  } else if (bytes_received < 0) {
    // std::cerr << "Recvfrom error" << std::endl;
    return -1;
  }
  return 0;
}

} // namespace configuration
} // namespace examples
} // namespace fastdds
} // namespace eprosima

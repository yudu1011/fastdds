#include <iostream>
#include <netinet/in.h>

// #include "CLIParser.hpp"

#ifndef FASTDDS_EXAMPLES_CPP_CONFIGURATION__UDPRECIVER_HPP
#define FASTDDS_EXAMPLES_CPP_CONFIGURATION__UDPRECIVER_HPP
namespace eprosima {
namespace fastdds {
namespace examples {
namespace lidartest {
class UdpReciver
{
public:

  // UdpReciver(const char* groupIP, uint16_t port, uint16_t bufferlen);
  UdpReciver(std::string groupIP, const uint16_t port);

  ~UdpReciver();

  long run(char *buffer_, int size);

private:

  // char* buffer_;

  // uint16_t bufferlen_;

  int sock_;

  sockaddr_in sender_address_;

  socklen_t sender_address_len_;
  

};

} // namespace configuration
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_EXAMPLES_CPP_CONFIGURATION__SUBSCRIBERAPP_HPP

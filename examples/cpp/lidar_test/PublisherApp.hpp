/**
 * @file PublisherApp.hpp
 *
 */

#ifndef FASTDDS_EXAMPLES_CPP_LIDAR_TEST__PUBLISHERAPP_HPP
#define FASTDDS_EXAMPLES_CPP_LIDAR_TEST__PUBLISHERAPP_HPP


#include <condition_variable>
#include <random>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "Application.hpp"
#include "CLIParser.hpp"
#include "lidardata.hpp"
#include "UdpReciver.hpp"

using namespace eprosima::fastdds::dds;
using namespace Lidar;
namespace eprosima {
namespace fastdds {
namespace examples {
namespace lidartest {

class PublisherApp : public Application, public DataWriterListener
{
public:

    PublisherApp(
            const CLIParser::publisher_config& config);

    ~PublisherApp();

    //! Publisher matched method
    void on_publication_matched(
            DataWriter* writer,
            const PublicationMatchedStatus& info) override;

    //! Sample deadline missed method
    void on_offered_deadline_missed(
            DataWriter* writer,
            const OfferedDeadlineMissedStatus& status) override;

    //! Incompatible QoS method
    void on_offered_incompatible_qos(
            DataWriter* writer,
            const OfferedIncompatibleQosStatus& status) override;

    //! Liveliness lost method
    void on_liveliness_lost(
            DataWriter* writer,
            const LivelinessLostStatus& status) override;

    //! Un-acked sample removed method
    void on_unacknowledged_sample_removed(
            DataWriter* writer,
            const InstanceHandle_t& instance) override;

    //! Run publisher
    void run() override;

    //! Trigger the end of execution
    void stop() override;

    void ptf();

    void reverse(char* arr,int start, int len);

private:

    //! Return the current state of execution
    bool is_stopped();

    //! Publish a sample
    bool publish();
    
    // void write_randomData();

    void udprun();

    void writePacket(uint32_t pk_sum_);
    void writeHeader();

    timeval nowUTC_;
    timeval cycletime_;
    timeval starttime_;

    UdpReciver udpreciver_;

    LidarDataDetection lidar_data_;
    Header headers_;
    LidarDataDet lidardatadets_;
    // Suffix suffix_;
    Packet packets_[630];

    uint32_t packet_sum_;

    DataBlock datablocks_[25];
    ChannelData chaneldatas_[5];

    DomainParticipant* participant_;

    Publisher* publisher_;

    Topic* topic_;

    DataWriter* writer_;

    TypeSupport type_;

    std::condition_variable cv_;

    int32_t matched_;

    std::mutex mutex_;

    uint32_t period_ms_;

    uint16_t samples_;
    
    uint32_t msg_size;

    // std::mt19937 generator_;

    // std::uniform_int_distribution<int> distribution_;   

    std::atomic<bool> stop_;

    int16_t wait_;

    std::atomic<bool> pause_;

    char buffer_[1300];

    // int16_t recived_bytes_;

};

} // namespace lidartest
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_EXAMPLES_CPP_LIDAT_TEST__PUBLISHERAPP_HPP

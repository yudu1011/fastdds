// Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file PublisherApp.cpp
 *
 */

#include "PublisherApp.hpp"
#include <random>
#include <algorithm>
#include <condition_variable>
#include <csignal>
#include <stdexcept>
#include <thread>
#include <sys/time.h>
#include <cmath>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.hpp>
#include "lidardataPubSubTypes.hpp"

// #include <fastdds/dds/core/Time.hpp>
using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace lidartest {

PublisherApp::PublisherApp(
        const CLIParser::publisher_config& config)
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new LidarDataDetectionPubSubType())
    , matched_(0)
    , period_ms_(config.interval)
    , samples_(config.samples)
    , stop_(false)
    , wait_(config.wait)
    , udpreciver_(config.groupIP, config.port)
{   

    // Creat a generator

    // udpreciver_ = UdpReciver("224.0.2.3", 51213);
    // std::random_device rd;
    // std::mt19937 generator_(rd());
    // std::uniform_int_distribution<int> distribution_(0, 0xFF); // 从0到255的整数
    // Set up the data type with initial values
    // configuration_.index(0);
    // memcpy(configuration_.message().data(), "Configuration", strlen("Configuration") + 1);
    // msg_size = config.msg_size;

    // configuration_.data(std::vector<uint8_t>(msg_size, 0xAA));

    // buffer_[msg_size];

    // std::cout << sizeof(&buffer_);
    gettimeofday(&starttime_,nullptr);
    // Create the participant
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("Configuration_pub_participant");
    auto factory = DomainParticipantFactory::get_instance();
    if ( !RETCODE_OK == factory->load_XML_profiles_file("lidartest_profile.xml"))
    {
        throw std::runtime_error("xml load failed");
    }
    if (config.profile_participant.empty())
    {
        // Include Participant QoS
        pqos.setup_transports(config.transport);
        for (auto& transportDescriptor : pqos.transport().user_transports)
        {
            SocketTransportDescriptor* pT = dynamic_cast<SocketTransportDescriptor*>(transportDescriptor.get());
            if (pT)
            {
                pT->TTL = config.ttl;
            }
        }
        participant_ = factory->create_participant(config.domain, pqos);
    }
    else
    {
        participant_ = factory->create_participant_with_profile(config.profile_participant);
    }
    if (participant_ == nullptr)
    {
        throw std::runtime_error("Participant initialization failed");
    }

    // Register the type
    type_.register_type(participant_);

    // Create the publisher
    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    if (!config.partitions.empty())
    {
        std::stringstream partitions(config.partitions);
        std::string partition;
        while (std::getline(partitions, partition, ';'))
        {
            pub_qos.partition().push_back(partition.c_str());
        }
    }
    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        throw std::runtime_error("Publisher initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(config.topic_name, type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create the data writer
    if (config.profile_writer.empty())
    {
        DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
        publisher_->get_default_datawriter_qos(writer_qos);
        writer_qos.publish_mode().kind = config.publish_mode;
        writer_qos.reliability().kind = config.reliability;
        writer_qos.durability().kind = config.durability;
        writer_qos.history().kind = config.history_kind;
        writer_qos.history().depth = config.history_depth;
        writer_qos.resource_limits().max_samples = config.max_samples;
        writer_qos.resource_limits().max_instances = config.max_instances;
        writer_qos.resource_limits().max_samples_per_instance = config.max_samples_per_instance;
        writer_qos.ownership().kind = config.ownership;
        if (config.ownership_strength > 0
                && config.ownership != OwnershipQosPolicyKind::EXCLUSIVE_OWNERSHIP_QOS)
        {
            throw std::runtime_error(
                      "DataWriter initialization failed: ownership strength is only valid with exclusive ownership");
        }
        writer_qos.ownership_strength().value = config.ownership_strength;
        if (config.deadline > 0)
        {
            writer_qos.deadline().period = eprosima::fastdds::dds::Duration_t(config.deadline * 1e-3);
        }
        writer_qos.reliable_writer_qos().disable_positive_acks.enabled = config.disable_positive_ack;
        if (config.ack_keep_duration > 0)
        {
            writer_qos.reliable_writer_qos().disable_positive_acks.duration = eprosima::fastdds::dds::Duration_t(
                config.ack_keep_duration * 1e-3);
        }
        if (config.lifespan > 0)
        {
            writer_qos.lifespan().duration = eprosima::fastdds::dds::Duration_t(config.lifespan * 1e-3);
        }
        writer_qos.liveliness().kind = config.liveliness;
        if (config.liveliness_lease > 0)
        {
            writer_qos.liveliness().lease_duration = eprosima::fastdds::dds::Duration_t(config.liveliness_lease * 1e-3);
        }
        if (config.liveliness_assert > 0)
        {
            writer_qos.liveliness().announcement_period = eprosima::fastdds::dds::Duration_t(
                config.liveliness_assert * 1e-3);
        }
        writer_ = publisher_->create_datawriter(topic_, writer_qos, this, StatusMask::all());
    }
    else
    {
        writer_ = publisher_->create_datawriter_with_profile(topic_, config.profile_writer, this, StatusMask::all());
    }
    if (writer_ == nullptr)
    {
        throw std::runtime_error("DataWriter initialization failed");
    }
}

PublisherApp::~PublisherApp()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void PublisherApp::on_publication_matched(
        eprosima::fastdds::dds::DataWriter* /*writer*/,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.current_count;
        std::cout << "Publisher matched." << std::endl;
        if (matched_ >= static_cast<int16_t>(wait_))
        {
            cv_.notify_one();
        }
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.current_count;
        std::cout << "Publisher unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void PublisherApp::on_offered_deadline_missed(
        eprosima::fastdds::dds::DataWriter* /*writer*/,
        const OfferedDeadlineMissedStatus& /*status*/)
{
    std::cout << "Deadline missed!" << std::endl;
}

void PublisherApp::on_offered_incompatible_qos(
        DataWriter* /*writer*/,
        const OfferedIncompatibleQosStatus& /*status*/)
{
    std::cout << "Incompatible QoS detected!" << std::endl;
}

void PublisherApp::on_liveliness_lost(
        DataWriter* /*writer*/,
        const LivelinessLostStatus& /*status*/)
{
    std::cout << "Liveliness lost!" << std::endl;
}

void PublisherApp::on_unacknowledged_sample_removed(
        DataWriter* /*writer*/,
        const InstanceHandle_t& /*instance*/)
{
    std::cout << "Unacknowledged sample was removed!" << std::endl;
}

void PublisherApp::run()
{
    packet_sum_ = 0;

    std::thread udprecive([&]() {this->udprun();});
    udprecive.detach();

    while (!is_stopped() && ((samples_ == 0)))
    {
        // printf("running\n");
        if (publish())
        {   
            std::cout  << static_cast<int>(sizeof(lidar_data_)) << " Bytes) SENT " << "Packet sum: " << lidar_data_.lidardatadet_().packet_sum() <<std::endl;
        }
        // Wait for period or stop event
        // std::unique_lock<std::mutex> terminate_lock(mutex_);
        // cv_.wait_for(terminate_lock, std::chrono::milliseconds(period_ms_), [&]()
        //     {
        //         return is_stopped();
        //     });
    }
}

bool PublisherApp::publish()
{
    // printf("publish running\n");
    bool ret = false;
    std::unique_lock<std::mutex> matched_lock(mutex_);
    // Wait for the data endpoints discovery
    // cv_.wait_for(matched_lock, std::chrono::milliseconds(period_ms_), [&]()
    //     {
    //         return ( pause_ && (matched_ >= static_cast<int16_t>(wait_)) || is_stopped());
    //     });
    cv_.wait(matched_lock,[&]()
    {
        return ((pause_ && (matched_ >= static_cast<int16_t>(wait_))) || is_stopped());
    });
    // std::cout << static_cast<bool>(matched_ >= static_cast<int16_t>(wait_))<< std::endl;
    if (!is_stopped() && pause_)
    {  
        // lidar_data_.data(std::vector<unsigned char>(buffer_, buffer_ + strlen(buffer_)));
        writeHeader();
        ret = (RETCODE_OK == writer_->write(&lidar_data_));
        packet_sum_ = 0;
        pause_.store(false);
        cv_.notify_one();
    }
    return ret;
}

void PublisherApp::udprun()
{   
    // printf("udpruning\n");
    std::unique_lock<std::mutex> lock(mutex_);
    while(true)
    {
        // if (!is_stopped())
        // {
            // printf("udpruning, packetnum=%d\n",packet_sum_);

        if (udpreciver_.run(buffer_, 1300) == 1218) 
        {               
            cv_.wait(lock, [&]{ return !pause_.load() || is_stopped(); });
            if (is_stopped()) break;
            // std::lock_guard (mutex_,);
            // 确保只当接收到正确数量的数据时才增加 packet_sum_
            packet_sum_++;
            // printf("%d /",packet_sum_.load());
            if (packet_sum_ == 630) 
            {
                pause_.store(true);
                cv_.notify_one();
            }
            writePacket(packet_sum_ - 1); // 处理接收到的数据包
        } 
        else 
        {
            std::cout << "Received data != 1218 bytes" << std::endl;
        }
        // }
        // else
        // {
            // std::this_thread::sleep_for(std::chrono::milliseconds(5));
            // break;
        // }
    }       
}


void PublisherApp::writePacket(uint32_t pk_sum_)
{
    int start_index;
    int start_ch;
    float radius;
    float elevation;
    float azimuth;

    reverse(buffer_, 20, sizeof(uint32_t));
    reverse(buffer_, 24, sizeof(uint32_t));
    memcpy(&lidar_data_.lidardatadet_().packet_list()[pk_sum_].timestamp_secs(), &buffer_[20], sizeof(uint32_t));
    // printf("%d,%x %x %x %x\n",lidar_data_.lidardatadet_().packet_list()[pk_sum_].timestamp_secs(),buffer_[20],buffer_[21],buffer_[22],buffer_[23]);
    memcpy(&lidar_data_.lidardatadet_().packet_list()[pk_sum_].timestamp_usecs(), &buffer_[24], sizeof(uint32_t));
    for (int i = 0; i < 25; i++ )
    {   
        start_index = 47 * i + 40;
        lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_sum() = i + 1;
        lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].time_offset() = buffer_[start_index];
        lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].return_seq() = buffer_[start_index + 1];
        // memcpy(&lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].time_offset(), &buffer_[start], 1);
        // memcpy(&lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].return_seq(), &buffer_[start + 1], 1);

        for (int j = 0; j < 5; j++ )
        {   
            start_ch = start_index + 2 + 9 * j;

            radius = float(static_cast<unsigned int>(buffer_[start_ch]) << 8 | static_cast<unsigned int>(buffer_[start_ch + 1])) * 0.005;
            elevation = (float(static_cast<unsigned int>(buffer_[start_ch + 2]) * 256 + static_cast<unsigned int>(buffer_[start_ch + 3]))- 32768.0) * 0.01;
            azimuth = (float(static_cast<unsigned int>(buffer_[start_ch + 3]) * 256 + static_cast<unsigned int>(buffer_[start_ch + 4]))- 32768.0) * 0.01;

            lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_sum() = j + 1;
            lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].x() = radius * std::cos(elevation) * std::cos(azimuth);
            // printf("first: %x; second: %x; last: %x; xvalue: %f \n", static_cast<unsigned int>(buffer_[start_ch + 2]),static_cast<unsigned int>(buffer_[start_ch + 3]),
            lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].y() = radius * std::cos(elevation) * std::sin(azimuth);
            lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].z() = radius * std::sin(elevation);         
            lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].intensity() = buffer_[start_ch + 6];   
            // printf("x= %.2f,y= %.2f, z= %.2f, i= %d\n",
            // lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].x(),
            // lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].y(),
            // lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].z(),
            // lidar_data_.lidardatadet_().packet_list()[pk_sum_].data_block_list()[i].channel_data_list()[j].intensity());                  
        }
    }
}

void PublisherApp::writeHeader()

{   
    gettimeofday(&nowUTC_,NULL);
    cycletime_.tv_sec = nowUTC_.tv_sec - starttime_.tv_sec;
    // memcpy(&lidar_data_.header_().seq(), &buffer_[8], sizeof(uint32_t));
    reverse(buffer_, 20, sizeof(uint32_t));
    reverse(buffer_, 24, sizeof(uint32_t));
    memcpy(&lidar_data_.header_().timestamp_sec(), &buffer_[20], sizeof(uint32_t));
    memcpy(&lidar_data_.header_().timestamp_usec(), &buffer_[24], sizeof(uint32_t));
    // printf("%d,%x %x %x %x\n",lidar_data_.header_().timestamp_sec(),buffer_[20],buffer_[21],buffer_[22],buffer_[23]);
    memcpy(&lidar_data_.header_().utc_timestamp_sec(), &nowUTC_.tv_sec, sizeof(int32_t));
    // printf("%ld %d |", lidar_data_.header_().utc_timestamp_sec(),lidar_data_.header_().cycle_time());
    memcpy(&lidar_data_.header_().cycle_time(), &cycletime_.tv_sec, sizeof(int32_t));
    memcpy(&lidar_data_.lidardatadet_().packet_sum(),&packet_sum_, sizeof(uint16_t));
    reverse(buffer_, 14, sizeof(uint16_t));
    memcpy(&lidar_data_.lidardatadet_().protocol_version(), &buffer_[14], sizeof(uint16_t));
    // printf("%x \n",lidar_data_.lidardatadet_().protocol_version());
    lidar_data_.lidardatadet_().return_mode() = buffer_[16];
    lidar_data_.lidardatadet_().time_sync_mode() = buffer_[17];
    // memcpy(&lidar_data_.lidardatadet_().return_mode(), &buffer_[16], sizeof(uint8_t));
    // memcpy(&lidar_data_.lidardatadet_().time_sync_mode(), &buffer_[17], sizeof(uint8_t));
    // memcpy(&lidar_data_.lidardatadet_().frame_sync(), &buffer_[17], sizeof(uint8_t));
}

void PublisherApp::reverse(char* arr,int start, int len)
{   
    char tmp;
    int end = start + len - 1;
    for (int i = 0; i < (len / 2); i ++ )
    {   
        std::swap(arr[start], arr[end]);
        // tmp = arr[start];
        // arr[start] = arr[end];
        // arr[end] = tmp;
        start ++;
        end --;
    }
}

bool PublisherApp::is_stopped()
{
    return stop_.load();
}

void PublisherApp::stop()
{
    stop_.store(true);
    cv_.notify_one();
}

// void PublisherApp::write_randomData()
// {   
//     std::vector<uint8_t> data_(msg_size, 0);
//     uint8_t rm_number = distribution_(generator_);
//     for (uint32_t i =0; i < msg_size; i++){
//         data_[i] = rm_number;
//         rm_number = distribution_(generator_);
//     }
//     configuration_.data(data_);
// }
} // namespace lidar_test
} // namespace examples
} // namespace fastdds
} // namespace eprosima

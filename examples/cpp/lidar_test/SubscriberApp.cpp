/**
 * @file SubscriberApp.cpp
 *
 */

#include "SubscriberApp.hpp"

#include <condition_variable>
#include <stdexcept>
#include <thread>
#include <sys/time.h>

#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.hpp>

#include "Application.hpp"
#include "CLIParser.hpp"
#include "lidardataPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace lidartest {

SubscriberApp::SubscriberApp(
        const CLIParser::subscriber_config& config)
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new LidarDataDetectionPubSubType())
    , received_samples_(0)
    , losted_samples_(0)
    , samples_(config.samples)
{
    // Create the participant
    gettimeofday(&starttime_,NULL);
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("Configuration_sub_participant");
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

    // Create the subscriber
    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    if (!config.partitions.empty())
    {
        std::stringstream partitions(config.partitions);
        std::string partition;
        while (std::getline(partitions, partition, ';'))
        {
            sub_qos.partition().push_back(partition.c_str());
        }
    }
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        throw std::runtime_error("Subscriber initialization failed");
    }

    // Create the topic
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(config.topic_name, type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("Topic initialization failed");
    }

    // Create the data reader
    if (config.profile_reader.empty())
    {
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        subscriber_->get_default_datareader_qos(reader_qos);
        reader_qos.reliability().kind = config.reliability;
        reader_qos.durability().kind = config.durability;
        reader_qos.history().kind = config.history_kind;
        reader_qos.history().depth = config.history_depth;
        reader_qos.resource_limits().max_samples = config.max_samples;
        reader_qos.resource_limits().max_instances = config.max_instances;
        reader_qos.resource_limits().max_samples_per_instance = config.max_samples_per_instance;
        reader_qos.ownership().kind = config.ownership;
        if (config.deadline > 0)
        {
            reader_qos.deadline().period = eprosima::fastdds::dds::Duration_t(config.deadline * 1e-3);
        }
        reader_qos.reliable_reader_qos().disable_positive_acks.enabled = config.disable_positive_ack;
        if (config.lifespan > 0)
        {
            reader_qos.lifespan().duration = eprosima::fastdds::dds::Duration_t(config.lifespan * 1e-3);
        }
        reader_qos.liveliness().kind = config.liveliness;
        if (config.liveliness_lease > 0)
        {
            reader_qos.liveliness().lease_duration = eprosima::fastdds::dds::Duration_t(config.liveliness_lease * 1e-3);
        }
        if (config.liveliness_assert > 0)
        {
            reader_qos.liveliness().announcement_period = eprosima::fastdds::dds::Duration_t(
                config.liveliness_assert * 1e-3);
        }
        reader_ = subscriber_->create_datareader(topic_, reader_qos, this, StatusMask::all());
    }
    else
    {
        reader_ = subscriber_->create_datareader_with_profile(topic_, config.profile_reader, this, StatusMask::all());
    }
    if (reader_ == nullptr)
    {
        throw std::runtime_error("DataReader initialization failed");
    }
}

SubscriberApp::~SubscriberApp()
{
    if (nullptr != participant_)
    {
        // Delete DDS entities contained within the DomainParticipant
        participant_->delete_contained_entities();

        // Delete DomainParticipant
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

void SubscriberApp::on_subscription_matched(
        DataReader* /*reader*/,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void SubscriberApp::on_data_available(
        DataReader* reader)
{
    SampleInfo info;
    while ((!is_stopped()) && (RETCODE_OK == reader->take_next_sample(&lidar_data_, &info)))
    {
        if ((info.instance_state == ALIVE_INSTANCE_STATE) && info.valid_data)
        {
            received_samples_++;
            std::cout << "get sample" << received_samples_ << "; size: " << static_cast<int>(sizeof(lidar_data_))<< "Bytes)"<<std::endl;
            // std::cout << "Sample: '" << configuration_.message().data() << "' with index: '" <<
            //     configuration_.index() << "' (" << static_cast<int>(configuration_.data().size()) <<
            //     " Bytes) RECEIVED  " << static_cast<int>(losted_samples_) << " samples losted " <<std::endl;
            if ((samples_ > 0) && (received_samples_ >= samples_))
            {
                stop();
            }
        }
    }
}

void SubscriberApp::on_requested_deadline_missed(
        DataReader* /*reader*/,
        const RequestedDeadlineMissedStatus& /*status*/)
{
    std::cout << "Requested deadline missed!" << std::endl;
}

void SubscriberApp::on_liveliness_changed(
        DataReader* /*reader*/,
        const LivelinessChangedStatus& /*status*/)
{
    std::cout << "Liveliness changed!" << std::endl;
}

void SubscriberApp::on_sample_rejected(
        DataReader* /*reader*/,
        const SampleRejectedStatus& /*status*/)
{
    std::cout << "Sample rejected!" << std::endl;
}

void SubscriberApp::on_requested_incompatible_qos(
        DataReader* /*reader*/,
        const RequestedIncompatibleQosStatus& /*status*/)
{
    std::cout << "Incompatible QoS detected!" << std::endl;
}

void SubscriberApp::on_sample_lost(
        DataReader* /*reader*/,
        const SampleLostStatus& /*status*/)
{
    // std::cout << "Sample lost!" << std::endl;
    losted_samples_ ++;
}

void SubscriberApp::run()
{   
    std::thread t_modify([=]() { this->ptf(); });
    t_modify.detach();

    std::unique_lock<std::mutex> lock_(mutex_);
    cv_.wait(lock_, [&]
            {
                return is_stopped();
            });
}

void SubscriberApp::ptf() 
{
    while (true) 
    {
        if (received_samples_ > 0)
        {
            // std::cout << "Sample: '" << lidar_data_.message().data() << "' with index: '" <<
            //     lidar_data_.index() << "' (" << static_cast<int>(lidar_data_.data().size()) <<
            //     " Bytes) RECEIVED  " << static_cast<int>(losted_samples_) << " samples losted " <<std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // std::unique_lock<std::mutex> terminate_lock(mutex_);
        // cv_.wait_for(terminate_lock, std::chrono::milliseconds(1000), [&]()
        //         {
        //             return is_stopped();
        //         });
    }
}

bool SubscriberApp::is_stopped()
{
    return stop_.load();
}

void SubscriberApp::stop()
{   
    gettimeofday(&nowtime_,NULL);
    uint64_t cyclonetime = (nowtime_.tv_sec - starttime_.tv_sec) * 1000000 + (nowtime_.tv_usec - starttime_.tv_usec);

    std::cout << static_cast<int>(received_samples_) << " Samples Recived ," << static_cast<int>(losted_samples_) << " Samples Losted; " 
    << "Lost Samples Rate: "<< static_cast<float>( (received_samples_ == 0) ? 
        0.0 : 100 * float(losted_samples_) /float(received_samples_ + losted_samples_)) << "%  " << "Transfer Speed: " 
    << static_cast<float>(float(received_samples_) / float(cyclonetime / 1000000)) << "samples/s" << std::endl;

    stop_.store(true);
    cv_.notify_all();
}

} // namespace lidartest
} // namespace examples
} // namespace fastdds
} // namespace eprosima

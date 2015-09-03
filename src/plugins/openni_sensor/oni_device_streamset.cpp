#include "oni_device_streamset.hpp"
#include "oni_depthstream.hpp"
#include "oni_colorstream.hpp"
#include "oni_infrared_stream.hpp"
#include <Shiny.h>
#include <sstream>

namespace orbbec { namespace ni {

    device_streamset::device_streamset(std::string name,
                                       astra::PluginServiceProxy& pluginService,
                                       const char* uri)
        : pluginService_(pluginService),
          uri_(uri)
    {
        PROFILE_FUNC();
        uri_ = uri;
        pluginService_.create_stream_set(name.c_str(), streamSetHandle_);
    }

    device_streamset::~device_streamset()
    {
        PROFILE_FUNC();
        close();
        pluginService_.destroy_stream_set(streamSetHandle_);
    }

    astra_status_t device_streamset::open()
    {
        PROFILE_FUNC();
        if (isOpen_)
            return ASTRA_STATUS_SUCCESS;

        LOG_INFO("orbbec.ni.device_streamset", "opening device: %s", uri_.c_str());
        openni::Status rc =  oniDevice_.open(uri_.c_str());

        if (rc != openni::STATUS_OK)
        {
            LOG_WARN("orbbec.ni.device_streamset", "failed to open device: %s", openni::OpenNI::getExtendedError());
            return ASTRA_STATUS_DEVICE_ERROR;
        }

        LOG_INFO("orbbec.ni.device_streamset", "opened device: %s", uri_.c_str());

        open_sensor_streams();

        isOpen_ = true;

        return ASTRA_STATUS_SUCCESS;
    }

    astra_status_t device_streamset::close()
    {
        PROFILE_FUNC();
        if (!isOpen_)
            return ASTRA_STATUS_SUCCESS;

        close_sensor_streams();

        LOG_INFO("orbbec.ni.device_streamset", "closing oni device: %s", uri_.c_str());
        oniDevice_.close();

        isOpen_ = false;

        return ASTRA_STATUS_SUCCESS;
    }

    astra_status_t device_streamset::read()
    {
        PROFILE_BLOCK(streamset_read);
        if (!isOpen_ || niActiveStreams_.size() == 0)
            return ASTRA_STATUS_SUCCESS;

        int streamIndex = -1;
        int timeout = openni::TIMEOUT_NONE;
        int i = 0;
        openni::Status rc;

        std::stringstream str;

        for(auto s : astraActiveStreams_)
        {
            (str << s->description().type()) << " ";
        }

        //LOG_INFO("orbbec.ni.device_streamset", "streams active: %s", str.str().c_str());

        for(i = 0; i < niActiveStreams_.size(); i++)
        {
            rc = openni::OpenNI::waitForAnyStream(&niActiveStreams_.data()[i],
                                                  1,
                                                  &streamIndex,
                                                  timeout);
            if (streamIndex != -1)
            {
                auto stream = astraActiveStreams_[i];

                stream->read(frameIndex_);
                //LOG_WARN("orbbec.ni.device_streamset", "stream read: %u", stream->description().type());
            }
            else
            {
                auto stream = astraActiveStreams_[i];
                //LOG_INFO("orbbec.ni.device_streamset", "stream not read: %u, ec: %u", stream->description().type(), rc);
            }

            if (streamIndex == 0)
            {
                //only increment frameIndex with primary stream
                //TODO this won't work when streams have different target FPS
                frameIndex_++;
            }
        }

        // do
        // {
        //     rc = openni::OpenNI::waitForAnyStream(niActiveStreams_.data(),
        //                                           niActiveStreams_.size(),
        //                                           &streamIndex,
        //                                           timeout);
        //     if (streamIndex != -1)
        //     {
        //         auto stream = astraActiveStreams_[streamIndex];

        //         stream->read(frameIndex_);
        //         LOG_INFO("orbbec.ni.device_streamset", "stream read: %u", stream->description().type());
        //         i++;
        //     }

        //     if (streamIndex == 0)
        //     {
        //         //only increment frameIndex with primary stream
        //         //TODO this won't work when streams have different target FPS
        //         frameIndex_++;
        //     }
        // } while (i < niActiveStreams_.size() && rc == openni::STATUS_OK);

        //LOG_INFO("orbbec.ni.device_streamset", "read from %u streams, rc: %d ", i, rc);

        if (rc == openni::STATUS_TIME_OUT)
        {
            return ASTRA_STATUS_TIMEOUT;
        }

        return ASTRA_STATUS_SUCCESS;
    }

    void device_streamset::add_stream(stream* stream)
    {
        streams_.push_back(stream_ptr(stream));
    }

    astra_status_t device_streamset::open_sensor_streams()
    {
        PROFILE_FUNC();

        bool enableColor = true;
        if (enableColor && oniDevice_.hasSensor(openni::SENSOR_COLOR))
        {
            colorstream* stream = new colorstream(pluginService_,
                                                  streamSetHandle_,
                                                  oniDevice_, *this);

            astra_status_t rc = ASTRA_STATUS_SUCCESS;
            rc = stream->open();
            add_stream(stream);

            if ( rc != ASTRA_STATUS_SUCCESS)
                LOG_WARN("orbbec.ni.device_streamset", "unable to open openni color stream.");
        }

        if (oniDevice_.hasSensor(openni::SENSOR_DEPTH))
        {
            depthstream* stream = new depthstream(pluginService_,
                                                  streamSetHandle_,
                                                  oniDevice_, *this);

            astra_status_t rc = ASTRA_STATUS_SUCCESS;
            rc = stream->open();
            add_stream(stream);

            if (rc != ASTRA_STATUS_SUCCESS)
                LOG_WARN("orbbec.ni.device_streamset", "unable to open openni depth stream.");
        }

        if (oniDevice_.hasSensor(openni::SENSOR_IR))
        {
            infrared_stream* stream = new infrared_stream(pluginService_,
                                                          streamSetHandle_,
                                                          oniDevice_,
                                                          *this);

            astra_status_t rc = ASTRA_STATUS_SUCCESS;
            rc = stream->open();
            add_stream(stream);

            if (rc != ASTRA_STATUS_SUCCESS)
                LOG_WARN("orbbec.ni.device_streamset", "unable to open openni infrared stream.");
        }

        return ASTRA_STATUS_SUCCESS;
    }

    void device_streamset::on_started(stream* stream)
    {
        auto niHandle = stream->get_stream();

        LOG_INFO("orbbec.ni.device_streamset",
                 "adding stream type %u to active streams",
                 stream->description().type());

        auto it = std::find(niActiveStreams_.begin(), niActiveStreams_.end(), niHandle);

        if (it == niActiveStreams_.end())
        {
            niActiveStreams_.push_back(niHandle);
            astraActiveStreams_.push_back(stream);
        }
    }

    void device_streamset::on_stopped(stream* stream)
    {
        auto niHandle = stream->get_stream();

        LOG_INFO("orbbec.ni.device_streamset",
                 "removing stream type %u from active streams",
                 stream->description().type());

        auto it = std::find(niActiveStreams_.begin(), niActiveStreams_.end(), niHandle);
        auto it2 = std::find(astraActiveStreams_.begin(), astraActiveStreams_.end(), stream);

        if (it != niActiveStreams_.end())
        {
            assert(it2 != astraActiveStreams_.end());

            niActiveStreams_.erase(it);
            astraActiveStreams_.erase(it2);
        }
    }

    astra_status_t device_streamset::close_sensor_streams()
    {
        PROFILE_FUNC();
        streams_.clear();

        return ASTRA_STATUS_SUCCESS;
    }
}}
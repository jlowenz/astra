﻿#ifndef OPENNIPLUGIN_H
#define OPENNIPLUGIN_H

#include "PluginBase.h"

#include <OpenNI.h>

namespace sensekit
{
	namespace OpenNI
	{

		class OpenNIPlugin : PluginBase
		{
		public:
			OpenNIPlugin();
			~OpenNIPlugin();

			virtual void orbbec_plugin_init(MockContext* context, PluginService* pluginService) override;
			sensekit_status_t open_depth_stream();
			sensekit_status_t close_depth_stream();
			void set_new_buffer(buffer* nextBuffer);
			sensekit_status_t read_next_depth_frame(sensekit_depthframe_t* frame);
			virtual void orbbec_plugin_cleanup() override;
			virtual void temp_update() override;

		private:
			MockContext* m_context;
			PluginService* m_pluginService;
			openni::Device m_device;
			openni::VideoStream m_depthStream;
			openni::DeviceInfo m_deviceInfo;

			buffer* m_currentBuffer{ nullptr };
			sensekit_depthframe_t* m_currentFrame{ nullptr };
		};
	}
}

#endif /* OPENNIPLUGIN_H */
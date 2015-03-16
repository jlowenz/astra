﻿// Orbbec (c) 2015

#include <SenseKit.h>
#include <cstdio>
#include <iostream>

int main(int argc, char** argv)
{
    sensekit_sensor_t* sensor;
    sensekit_status_t status = SENSEKIT_STATUS_SUCCESS;

    status = sensekit_open_sensor("1d27/0601@20/30", &sensor);

    sensekit_depthstream_t* depthStream;
    status = sensekit_depth_open(sensor, &depthStream);

    char c = 0;

    do
    {
        sensekit_depthframe_t* depthFrame;
        sensekit_depth_frame_open(depthStream,
                                  30, &depthFrame);

        std::cout << "index: " << depthFrame->frameIndex << " value: " << depthFrame->sampleValue << std::endl;

        sensekit_depth_frame_close(&depthFrame);

    } while (c != 'q');

    status = sensekit_depth_close(&depthStream);

    status = sensekit_close_sensor(&sensor);

    sensekit_terminate();
}

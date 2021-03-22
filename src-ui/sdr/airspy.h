#pragma once

#include "sdr.h"
#include <libairspy/airspy.h>

class SDRAirspy : public SDRDevice
{
private:
    struct airspy_device *dev;
    int gain = 10;
    bool bias = false;
    int d_samplerate;
    int d_frequency;

public:
    SDRAirspy();
    void start();
    void stop();
    void drawUI();
};
#pragma once

#include "modules/buffer.h"

class SDRDevice
{
public:
    std::shared_ptr<dsp::stream<std::complex<float>>> output_stream;
    SDRDevice();
    virtual void start();
    virtual void stop();
    virtual void drawUI();
};
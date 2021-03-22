#include "fft.h"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "logger.h"
#include <complex>
#include <fstream>

libdsp::Pipe<std::complex<float>> fft_pipe;

static int _rx_callback(airspy_transfer *t)
{
    // ((satdump::Pipe *)t->ctx)->push((uint8_t *)t->samples, t->sample_count, sizeof(std::complex<float>));
    //logger->info(t->sample_count);
    //std::memcpy(((dsp::stream<std::complex<float>> *)t->ctx)->writeBuf, t->samples, t->sample_count * sizeof(std::complex<float>));
    //((dsp::stream<std::complex<float>> *)t->ctx)->swap(t->sample_count);
    //std::shared_ptr<dsp::stream<std::complex<float>>> pipe = *((std::shared_ptr<dsp::stream<std::complex<float>>> *)t->ctx);
    fft_pipe.push((std::complex<float> *)t->samples, t->sample_count);
    return 0;
};

SDRSource::SDRSource(int frequency, int samplerate, std::shared_ptr<dsp::stream<std::complex<float>>> output_pipe)
{
    d_samplerate = samplerate;
    d_frequency = frequency;
    d_output_pipe = output_pipe;
    logger->info("Using freq " + std::to_string(frequency));
    std::fill(&fft_buffer[0], &fft_buffer[2048], 0);
}

void SDRSource::startSDR()
{
    if (airspy_open(&dev) != AIRSPY_SUCCESS)
    {
        logger->error("Could not open Airspy device!");
    }

    logger->info("Opened Airspy device!");

    std::fill(frequency, &frequency[100], 0);

    std::memcpy(frequency, std::to_string(d_frequency / 1e6).c_str(), std::to_string(d_frequency / 1e6).length());

    airspy_set_sample_type(dev, AIRSPY_SAMPLE_FLOAT32_IQ);
    airspy_set_samplerate(dev, d_samplerate);
    airspy_set_freq(dev, d_frequency);
    airspy_set_rf_bias(dev, bias);
    airspy_set_linearity_gain(dev, gain);

    airspy_start_rx(dev, &_rx_callback, &d_output_pipe);

    fft_thread = std::thread(&SDRSource::fftFun, this);
}

void SDRSource::fftFun()
{

    int refresh_per_second = 20;
    int runs_to_wait = (d_samplerate / 8192) / (refresh_per_second * 3);
    int i = 0, y = 0, cnt = 0;

    float fftb[2048];
    int16_t buf16[8192 * 2];

    std::complex<float> sample_buffer[8192];
    std::complex<float> buffer_fft_out[2048];

    fftwf_plan p = fftwf_plan_dft_1d(2048, (fftwf_complex *)sample_buffer, (fftwf_complex *)buffer_fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

    // std::ifstream filein("/home/alan/Downloads/10-44-34_1701298418Hz(MetOp-C).wav");

    while (1)
    {
        cnt = fft_pipe.pop(sample_buffer, 8192, 1000);

        std::memcpy(d_output_pipe->writeBuf, sample_buffer, cnt * sizeof(std::complex<float>));
        d_output_pipe->swap(cnt);

        if (y % runs_to_wait == 0)
        {
            fftwf_execute(p);

            for (int i = 0, iMax = 2048 / 2; i < iMax; i++)
            {
                float a = buffer_fft_out[i].real();
                float b = buffer_fft_out[i].imag();
                float c = sqrt(a * a + b * b);

                float x = buffer_fft_out[2048 / 2 + i].real();
                float y = buffer_fft_out[2048 / 2 + i].imag();
                float z = sqrt(x * x + y * y);

                fftb[i] = z * 4.0f * scale;
                fftb[2048 / 2 + i] = c * 4.0f * scale;
            }

            for (int i = 0; i < 2048; i++)
            {
                fft_buffer[i] = (fftb[i] * 1 + fft_buffer[i] * 9) / 10;
            }

            i++;

            if (i == 10000000)
                i = 0;
        }

        if (y == 10000000)
            y = 0;
        y++;
    }
}

void SDRSource::drawUI()
{
    ImGui::Begin("Input FFT", NULL, NOWINDOW_FLAGS);

    ImGui::PlotLines("", fft_buffer, IM_ARRAYSIZE(fft_buffer), 0, 0, 0, 100, {ImGui::GetWindowWidth() - 3, ImGui::GetWindowHeight() - 64});

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stoi(frequency) * 1e6;
        airspy_set_freq(dev, d_frequency);
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2.0 - 165);
    if (ImGui::SliderInt("Gain", &gain, 0, 22))
    {
        airspy_set_linearity_gain(dev, gain);
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2.0 - 165);
    ImGui::SliderFloat("Scale", &scale, 0, 22);

    ImGui::SameLine();
    if (ImGui::Checkbox("Bias", &bias))
    {
        airspy_set_rf_bias(dev, bias);
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        airspy_stop_rx(dev);
        stopFuction();
    }

    ImGui::End();
}
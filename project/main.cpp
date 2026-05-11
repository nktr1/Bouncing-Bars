#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <ncurses.h>
#include "fftw3.h"
#include <cmath>
#include <unistd.h>
#include <cstring>

#define FFT_SIZE 1024
#define BARS     40

double fft_in[FFT_SIZE];
fftw_complex fft_out[FFT_SIZE/2 + 1];
fftw_plan plan;

volatile float visual_spectrum[BARS] = {0};
float temp_buffer[FFT_SIZE * 2];

volatile int callbacks = 0;
volatile int total_samples = 0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    callbacks++;
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (!pDecoder) return;

    float* output = (float*)pOutput;
    int channels = pDevice->playback.channels;

    ma_uint64 framesRead;
    ma_decoder_read_pcm_frames(pDecoder, output, frameCount, &framesRead);

    if (framesRead > 0) {
        int idx = 0;
        for (ma_uint64 i = 0; i < framesRead && idx < FFT_SIZE; i++) {
            float sample = 0.0f;
            for (int ch = 0; ch < channels; ch++) {
                sample += output[i * channels + ch];
            }
            sample /= channels;
            temp_buffer[idx++] = sample;
        }
        while (idx < FFT_SIZE) temp_buffer[idx++] = 0.0f;

        for (int j = 0; j < FFT_SIZE; j++) {
            double w = 0.5 * (1.0 - cos(2.0 * M_PI * j / (FFT_SIZE)));
            fft_in[j] = temp_buffer[j] * w;
        }

        fftw_execute(plan);

        for (int b = 0; b < BARS; b++) {
            double mag = sqrt(fft_out[b][0]*fft_out[b][0] + fft_out[b][1]*fft_out[b][1]);
            visual_spectrum[b] = (float)(mag * 4.5f / sqrt(FFT_SIZE));
        }

        total_samples += (int)framesRead;
    }
}

int main()
{
    initscr();
    noecho();
    curs_set(0);
    timeout(40);

    plan = fftw_plan_dft_r2c_1d(FFT_SIZE, fft_in, fft_out, FFTW_ESTIMATE);

    ma_decoder decoder;
    if (ma_decoder_init_file("track.mp3", NULL, &decoder) != MA_SUCCESS) {
        endwin();
        printf("Не удалось открыть track.mp3\n");
        return 1;
    }

    int sample_rate = decoder.outputSampleRate;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = decoder.outputFormat;
    config.playback.channels = decoder.outputChannels;
    config.sampleRate        = sample_rate;
    config.dataCallback      = data_callback;
    config.pUserData         = &decoder;

    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        endwin();
        printf("Ошибка audio устройства\n");
        ma_decoder_uninit(&decoder);
        return 1;
    }

    ma_device_set_master_volume(&device, 0.35f);
    ma_device_start(&device);

    while (true) {
        clear();
        mvprintw(0, 0, "MIPT Visualizer | Press 'q' to exit");

        float max_mag = 0;
        for (int i = 0; i < BARS; i++) {
            if (visual_spectrum[i] > max_mag) max_mag = visual_spectrum[i];
        }

        mvprintw(1, 0, "Sample Rate: %d Hz | Max mag: %.4f", sample_rate, max_mag);

        for (int i = 0; i < BARS; i++) {
            float val = visual_spectrum[i];
            int height = (int)(log10(val * 80.0f + 1.0f) * 11.0f);
            
            if (height > 23) height = 23;
            if (height < 0) height = 0;

            for (int y = 0; y < height; y++) {
                mvaddch(25 - y, i*2 + 2, '#');
            }
        }

        mvprintw(27, 0, "-----------------------------------------------------------------");
        
        for (int i = 0; i < BARS; i += 4) {
            float freq = (i * sample_rate) / (float)FFT_SIZE;
            int x_pos = i*2 + 1;
            
            if (freq < 1000) {
                mvprintw(28, x_pos, "%.0f", freq);
            } else {
                mvprintw(28, x_pos, "%.0fk", freq/1000);
            }
        }

        refresh();

        if (getch() == 'q') break;
        usleep(16000);
    }

    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
    fftw_destroy_plan(plan);
    endwin();

    return 0;
}

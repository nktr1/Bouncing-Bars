#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "fftw3.h"

#include <SFML/Graphics.hpp>

#include <cmath>
#include <cstring>
#include <vector>
#include <optional>

#define FFT_SIZE 1024
#define BARS 75

double fft_in[FFT_SIZE];
fftw_complex fft_out[FFT_SIZE/2 + 1];
fftw_plan plan;

volatile float visual_spectrum[BARS]={0};
float temp_buffer[FFT_SIZE*2];

float smooth[BARS]={0};

void data_callback(
    ma_device* pDevice,
    void* pOutput,
    const void* pInput,
    ma_uint32 frameCount)
{
    ma_decoder* pDecoder =
        (ma_decoder*)pDevice->pUserData;

    if(!pDecoder) return;

    float* output=(float*)pOutput;

    int channels=
        pDevice->playback.channels;

    ma_uint64 framesRead;

    ma_decoder_read_pcm_frames(
        pDecoder,
        output,
        frameCount,
        &framesRead
    );

    if(framesRead>0)
    {
        int idx=0;

        for(ma_uint64 i=0;
            i<framesRead && idx<FFT_SIZE;
            i++)
        {
            float sample=0;

            for(int ch=0;
                ch<channels;
                ch++)
            {
                sample +=
                output[i*channels+ch];
            }

            sample/=channels;

            temp_buffer[idx++]=sample;
        }

        while(idx<FFT_SIZE)
            temp_buffer[idx++]=0;

        for(int i=0;i<FFT_SIZE;i++)
        {
            double w=0.5*(1-cos(2*M_PI*i/FFT_SIZE));

            fft_in[i]=temp_buffer[i]*w;
        }

        fftw_execute(plan);

        for(int b=0;b<BARS;b++)
        {
            float minFreq = 20.0f;
            float maxFreq = 20000.0f;

            float t = (float)b / (float)(BARS - 1);

             
            float freq = minFreq * pow(maxFreq / minFreq, t);

             
            int bin = (int)(freq / (44100.0f / FFT_SIZE));

            if(bin >= FFT_SIZE/2)
                bin = FFT_SIZE/2 - 1;

            double mag=sqrt(
            fft_out[bin][0]*
            fft_out[bin][0]+
            fft_out[bin][1]*
            fft_out[bin][1]);

            visual_spectrum[b]=
            mag*6.0f/sqrt(FFT_SIZE);
        }
    }
}

int main()
{
    plan=fftw_plan_dft_r2c_1d(
        FFT_SIZE,
        fft_in,
        fft_out,
        FFTW_ESTIMATE);

    ma_decoder decoder;

    if(ma_decoder_init_file("track.mp3",NULL,&decoder)!=MA_SUCCESS)
    {
        printf("track.mp3 not found\n");
        return 1;
    }

    ma_device_config config=ma_device_config_init(ma_device_type_playback);

    config.playback.format=decoder.outputFormat;

    config.playback.channels=decoder.outputChannels;

    config.sampleRate=decoder.outputSampleRate;

    config.dataCallback=data_callback;

    config.pUserData=&decoder;

    ma_device device;

    if(ma_device_init(NULL,&config,&device)!=MA_SUCCESS)
    {
        return 1;
    }

    ma_device_start(&device);

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1400,800)),"FFT Visualizer");

    window.setFramerateLimit(60);

    float width=1400.f/BARS;

    while(window.isOpen())
    {
        while(auto event=
              window.pollEvent())
        {
            if(event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if(auto* key=
                event->getIf<
                sf::Event::KeyPressed>())
            {
                if(key->code==
                   sf::Keyboard::Key::Q)
                {
                    window.close();
                }
            }
        }

        window.clear(sf::Color(15,15,20));

        for(int i=0;i<BARS;i++)
        {
            float value=visual_spectrum[i];

            value=log10(value*90+1)*250;

            if(value>700)value=700;

            smooth[i]+=(value-smooth[i])*0.2f;

            float h=smooth[i];

            sf::RectangleShape bar;

            bar.setSize({width-4,h});

            bar.setPosition({i*width,780-h});

            int r=std::min(255,(int)(50+h/3));

            int g = std::min(255,(int)(100+h/2));

            int b=255;

            bar.setFillColor(sf::Color(r,g,b));

            window.draw(bar);
        }

        window.display();
    }

    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);

    fftw_destroy_plan(plan);

    return 0;
}

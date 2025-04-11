#ifndef AUDIO_H
#define AUDIO_H 
#ifdef WIN
    #include <windows.h>
    #include <mmsystem.h>
    #include <mmreg.h>
    #pragma comment(lib, "winmm.lib")
    #define NUM_CHANNELS 8
    #define BUFFER_SAMPLES 1024
    #define NUM_BUFFERS 4
    #define SAMPLE_RATE 32000
    #define CHANNEL_COUNT 1
    typedef struct {
        float* samples;
        unsigned int sample_count;
        unsigned int cur_sample;
        int playing;
    } SoundChannel;
    typedef struct {
        WAVEHDR header;
        float buffer[BUFFER_SAMPLES];
    } AudioBuffer;
    typedef struct CachedSound {
        char filename[256];
        float* samples;
        unsigned int sample_count;
        struct CachedSound* next;
    } CachedSound;
    static int audioSystemInitialized = 0;
    static HWAVEOUT hWaveOut = NULL;
    static SoundChannel channels[NUM_CHANNELS];
    static AudioBuffer audioBuffers[NUM_BUFFERS];
    static CachedSound* soundCache = NULL;
    void initializeAudio(void);
    void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void MixAudio(float* outBuffer, int numSamples);
    CachedSound* loadWavFromFile(const char* filename);
    CachedSound* findCachedSound(const char* filename);
    void addCachedSound(CachedSound* cs);
    void cleanupAudio(void);
    float* convertWavToStandard(void* rawData, unsigned int totalSamples, WORD audioFormat, WORD bitsPerSample, WORD numChannels, DWORD sampleRate, unsigned int* outSampleCount);
    void MixAudio(float* outBuffer, int numSamples) {
        for (int i = 0; i < numSamples; i++) {
            float mixed = 0.0f;
            for (int ch = 0; ch < NUM_CHANNELS; ch++) {
                if (channels[ch].playing) {
                    if (channels[ch].cur_sample < channels[ch].sample_count) {
                        mixed += channels[ch].samples[channels[ch].cur_sample];
                        channels[ch].cur_sample++;
                    } else {
                        channels[ch].playing = 0;
                    }
                }
            }
            if (mixed > 1.0f)
                mixed = 1.0f;
            else if (mixed < -1.0f)
                mixed = -1.0f;
            outBuffer[i] = mixed;
        }
    }
    void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
        if (uMsg == WOM_DONE) {
            WAVEHDR* header = (WAVEHDR*)dwParam1;
            AudioBuffer* audioBuf = (AudioBuffer*)header->dwUser;
            MixAudio(audioBuf->buffer, BUFFER_SAMPLES);
            waveOutWrite(hWaveOut, header, sizeof(WAVEHDR));
        }
    }
    float* convertWavToStandard(void* rawData, unsigned int totalSamples, WORD audioFormat, WORD bitsPerSample, WORD numChannels, DWORD sampleRate, unsigned int* outSampleCount) {
        unsigned int inputFrames = totalSamples / numChannels;
        float* monoBuffer = (float*)malloc(sizeof(float) * inputFrames);
        if (!monoBuffer) return NULL;
        for (unsigned int i = 0; i < inputFrames; i++) {
            float sum = 0.0f;
            for (int ch = 0; ch < numChannels; ch++) {
                if (audioFormat == 1) {
                    if (bitsPerSample == 8) {
                        unsigned char sample = ((unsigned char*)rawData)[i * numChannels + ch];
                        float f = (((int)sample - 128) / 128.0f);
                        sum += f;
                    } else if (bitsPerSample == 16) {
                        short sample = ((short*)rawData)[i * numChannels + ch];
                        float f = sample / 32768.0f;
                        sum += f;
                    } else if (bitsPerSample == 32) {
                        int sample = ((int*)rawData)[i * numChannels + ch];
                        float f = sample / 2147483648.0f;
                        sum += f;
                    } else {
                        sum += 0.0f;
                    }
                } else if (audioFormat == 3) {
                    float sample = ((float*)rawData)[i * numChannels + ch];
                    sum += sample;
                } else {
                    sum += 0.0f;
                }
            }
            monoBuffer[i] = sum / numChannels;
        }
        if (sampleRate == SAMPLE_RATE) {
            *outSampleCount = inputFrames;
            return monoBuffer;
        } else {
            unsigned int targetFrames = (unsigned int)(inputFrames * ((float)SAMPLE_RATE / sampleRate));
            float* resampled = (float*)malloc(sizeof(float) * targetFrames);
            if (!resampled) {
                free(monoBuffer);
                return NULL;
            }
            for (unsigned int i = 0; i < targetFrames; i++) {
                float pos = ((float)i * sampleRate) / SAMPLE_RATE;
                unsigned int idx = (unsigned int)pos;
                float frac = pos - idx;
                float sample1 = (idx < inputFrames) ? monoBuffer[idx] : 0.0f;
                float sample2 = ((idx + 1) < inputFrames) ? monoBuffer[idx + 1] : 0.0f;
                resampled[i] = sample1 * (1.0f - frac) + sample2 * frac;
            }
            free(monoBuffer);
            *outSampleCount = targetFrames;
            return resampled;
        }
    }
    CachedSound* loadWavFromFile(const char* filename) {
        FILE* f = fopen(filename, "rb");
        if (!f) {
            printf("Error opening file: %s\n", filename);
            return NULL;
        }
        char riff[4];
        fread(riff, 1, 4, f);
        if (strncmp(riff, "RIFF", 4) != 0) {
            printf("Not a valid RIFF file: %s\n", filename);
            fclose(f);
            return NULL;
        }
        fseek(f, 8, SEEK_SET);
        char waveTag[4];
        fread(waveTag, 1, 4, f);
        if (strncmp(waveTag, "WAVE", 4) != 0) {
            printf("Not a valid WAVE file: %s\n", filename);
            fclose(f);
            return NULL;
        }
        WORD audioFormat = 0, numChannels = 0, bitsPerSample = 0;
        DWORD sampleRate = 0;
        DWORD dataSize = 0;
        int fmtFound = 0, dataFound = 0;
        while (!dataFound && !feof(f)) {
            char chunkId[5] = {0};
            DWORD chunkSize = 0;
            if (fread(chunkId, 1, 4, f) != 4)
                break;
            fread(&chunkSize, sizeof(DWORD), 1, f);
            if (strncmp(chunkId, "fmt ", 4) == 0) {
                fmtFound = 1;
                fread(&audioFormat, sizeof(WORD), 1, f);
                fread(&numChannels, sizeof(WORD), 1, f);
                fread(&sampleRate, sizeof(DWORD), 1, f);
                fseek(f, 6, SEEK_CUR);
                fread(&bitsPerSample, sizeof(WORD), 1, f);
                if (chunkSize > 16)
                    fseek(f, chunkSize - 16, SEEK_CUR);
            } else if (strncmp(chunkId, "data", 4) == 0) {
                dataFound = 1;
                dataSize = chunkSize;
                break;
            } else {
                fseek(f, chunkSize, SEEK_CUR);
            }
        }
        if (!fmtFound || !dataFound) {
            printf("Failed to find necessary chunks in file: %s\n", filename);
            fclose(f);
            return NULL;
        }
        unsigned int totalSamples = dataSize * 8 / bitsPerSample;
        void* rawData = malloc(dataSize);
        if (!rawData) {
            printf("Memory allocation failed for file: %s\n", filename);
            fclose(f);
            return NULL;
        }
        fread(rawData, 1, dataSize, f);
        fclose(f);
        unsigned int standardSampleCount = 0;
        float* standardSamples = convertWavToStandard(rawData, totalSamples, audioFormat, bitsPerSample, numChannels, sampleRate, &standardSampleCount);
        free(rawData);
        if (!standardSamples) {
            printf("Conversion to standard format failed for file: %s\n", filename);
            return NULL;
        }
        CachedSound* cs = (CachedSound*)malloc(sizeof(CachedSound));
        if (!cs) {
            free(standardSamples);
            return NULL;
        }
        strncpy(cs->filename, filename, 255);
        cs->filename[255] = '\0';
        cs->samples = standardSamples;
        cs->sample_count = standardSampleCount;
        cs->next = NULL;
        return cs;
    }
    CachedSound* findCachedSound(const char* filename) {
        CachedSound* cs = soundCache;
        while (cs) {
            if (strcmp(cs->filename, filename) == 0)
                return cs;
            cs = cs->next;
        }
        return NULL;
    }
    void addCachedSound(CachedSound* cs) {
        cs->next = soundCache;
        soundCache = cs;
    }
    void initializeAudio(void)
    {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            channels[i].samples = NULL;
            channels[i].sample_count = 0;
            channels[i].cur_sample = 0;
            channels[i].playing = 0;
        }
        WAVEFORMATEX wf;
        wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wf.nChannels = CHANNEL_COUNT;
        wf.nSamplesPerSec = SAMPLE_RATE;
        wf.wBitsPerSample = 32;
        wf.nBlockAlign = (wf.nChannels * wf.wBitsPerSample) / 8;
        wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
        wf.cbSize = 0;
        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wf, (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            printf("Failed to open wave output.\n");
            exit(1);
        }
        for (int i = 0; i < NUM_BUFFERS; i++) {
            memset(&audioBuffers[i], 0, sizeof(AudioBuffer));
            audioBuffers[i].header.lpData = (LPSTR)audioBuffers[i].buffer;
            audioBuffers[i].header.dwBufferLength = BUFFER_SAMPLES * sizeof(float);
            audioBuffers[i].header.dwFlags = 0;
            audioBuffers[i].header.dwUser = (DWORD_PTR)&audioBuffers[i];
            waveOutPrepareHeader(hWaveOut, &audioBuffers[i].header, sizeof(WAVEHDR));
            MixAudio(audioBuffers[i].buffer, BUFFER_SAMPLES);
            waveOutWrite(hWaveOut, &audioBuffers[i].header, sizeof(WAVEHDR));
        }
        audioSystemInitialized = 1;
    }
    void playSoundLazy(const char* filename)
    {
        if(!audioSystemInitialized){initializeAudio();}
        CachedSound* cs = findCachedSound(filename);
        if (!cs) {
            cs = loadWavFromFile(filename);
            if (!cs) {
                printf("Failed to load sound: %s\n", filename);
                return;
            }
            addCachedSound(cs);
        }
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i].playing) {
                channels[i].samples = cs->samples;
                channels[i].sample_count = cs->sample_count;
                channels[i].cur_sample = 0;
                channels[i].playing = 1;
                return;
            }
        }
        printf("No free channel available to play sound: %s\n", filename);
    }
    void playSound(const char* filename)
    {
        CachedSound* cs = findCachedSound(filename);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i].playing) {
                channels[i].samples = cs->samples;
                channels[i].sample_count = cs->sample_count;
                channels[i].cur_sample = 0;
                channels[i].playing = 1;
                return;
            }
        }
    }
    void cacheSound(const char* filename)
    {
        CachedSound* cs = loadWavFromFile(filename);
        if (!cs) {
            printf("Failed to load sound: %s\n", filename);
            return;
        }
        addCachedSound(cs);
    }
    void cleanupAudio(void) {
        if (hWaveOut) {
            waveOutReset(hWaveOut);
            for (int i = 0; i < NUM_BUFFERS; i++) {
                waveOutUnprepareHeader(hWaveOut, &audioBuffers[i].header, sizeof(WAVEHDR));
            }
            waveOutClose(hWaveOut);
        }
        CachedSound* cs = soundCache;
        while (cs) {
            CachedSound* next = cs->next;
            if (cs->samples)
                free(cs->samples);
            free(cs);
            cs = next;
        }
    }
#endif
#ifdef SDL_AUDIO
    #define MAX_CHANNELS 8
    typedef struct {
        Uint8 *buffer;
        Uint32 length;
        Uint32 position;
        int inUse;
    } AudioChannel;
    AudioChannel channels[MAX_CHANNELS] = {0};
    SDL_AudioDeviceID gDevice;
    SDL_AudioSpec gAudioSpec;
    void audio_callback(void *userdata, Uint8 *stream, int len)
    {
        SDL_memset(stream, 0, len);
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (channels[i].inUse) {
                Uint32 remaining = channels[i].length - channels[i].position;
                Uint32 bytesToMix = (remaining > (Uint32)len ? len : remaining);
                SDL_MixAudioFormat(stream,
                                    channels[i].buffer + channels[i].position,
                                    gAudioSpec.format,
                                    bytesToMix,
                                    SDL_MIX_MAXVOLUME);
                channels[i].position += bytesToMix;
                if (channels[i].position >= channels[i].length) {
                    free(channels[i].buffer);
                    channels[i].buffer = NULL;
                    channels[i].length = 0;
                    channels[i].position = 0;
                    channels[i].inUse = 0;
                }
            }
        }
    }
    void playSound(const char *filename)
    {
        SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
        if (!rw) {
            fprintf(stderr, "Failed to open file: %s\n", filename);
            return;
        }
        SDL_AudioSpec wavSpec;
        Uint8 *wavBuffer;
        Uint32 wavLength;
        if (SDL_LoadWAV_RW(rw, 1, &wavSpec, &wavBuffer, &wavLength) == NULL) {
            fprintf(stderr, "Failed to load WAV: %s\n", SDL_GetError());
            return;
        }
        SDL_AudioCVT cvt;
        Uint8 *convertedBuffer = NULL;
        Uint32 convertedLength = 0;
        if (SDL_BuildAudioCVT(&cvt,
                            wavSpec.format, wavSpec.channels, wavSpec.freq,
                            gAudioSpec.format, gAudioSpec.channels, gAudioSpec.freq) < 0) {
            fprintf(stderr, "Audio conversion error: %s\n", SDL_GetError());
            SDL_FreeWAV(wavBuffer);
            return;
        }
        if (cvt.needed) {
            cvt.len = wavLength;
            convertedBuffer = (Uint8 *)malloc(cvt.len * cvt.len_mult);
            if (!convertedBuffer) {
                fprintf(stderr, "Memory allocation error\n");
                SDL_FreeWAV(wavBuffer);
                return;
            }
            memcpy(convertedBuffer, wavBuffer, wavLength);
            cvt.buf = convertedBuffer;
            if (SDL_ConvertAudio(&cvt) < 0) {
                fprintf(stderr, "SDL_ConvertAudio failed: %s\n", SDL_GetError());
                free(convertedBuffer);
                SDL_FreeWAV(wavBuffer);
                return;
            }
            convertedLength = cvt.len_cvt;
            SDL_FreeWAV(wavBuffer);
        } else {
            convertedBuffer = wavBuffer;
            convertedLength = wavLength;
        }
        SDL_LockAudioDevice(gDevice);
        int slotFound = 0;
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if (!channels[i].inUse) {
                channels[i].inUse = 1;
                channels[i].buffer = convertedBuffer;
                channels[i].length = convertedLength;
                channels[i].position = 0;
                slotFound = 1;
                break;
            }
        }
        SDL_UnlockAudioDevice(gDevice);
        if (!slotFound) {
            fprintf(stderr, "No available channels for sound: %s\n", filename);
            free(convertedBuffer);
        }
    }
#endif
#endif

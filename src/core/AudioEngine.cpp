#include "core/AudioEngine.h"
#include "core/SpectrumAnalyzer.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <QDebug>
#include <QTimer>
#include <QtGlobal>

#include <algorithm>
#include <atomic>
#include <vector>

namespace Velvox {

namespace {

constexpr int kDefaultPositionIntervalMs = 200;

} // anonymous namespace

class AudioEngine::Impl
{
public:
    AudioEngine *q = nullptr;

    ma_decoder decoder{};
    ma_device device{};
    std::unique_ptr<SpectrumAnalyzer> analyzer;

    std::atomic<bool> finishedFlag{false};
    std::atomic<float> volume{1.0f};
    std::atomic<bool> spectrumEnabled{true};

    // Pre-allocated buffer for mono samples fed to the spectrum analyzer.
    // Avoids heap allocation inside the real-time audio callback.
    std::vector<float> spectrumMonoBuffer;

    PlaybackState state = PlaybackState::Stopped;
    QString currentPath;
    qint64 durationMs = 0;
    int positionIntervalMs = kDefaultPositionIntervalMs;

    QTimer *positionTimer = nullptr;

    bool initDevice();
    void updateState(PlaybackState newState);
    void close();

    static void dataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
};

void AudioEngine::Impl::dataCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    Q_UNUSED(pInput)

    auto *impl = static_cast<Impl *>(pDevice->pUserData);
    if (!impl)
        return;

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&impl->decoder, pOutput, frameCount, &framesRead);

    const ma_uint32 channels = impl->decoder.outputChannels;
    const float volume = impl->volume.load();

    // Apply volume in the audio callback (no per-file mixer yet, so this is app-level).
    float *output = static_cast<float *>(pOutput);
    for (ma_uint64 i = 0; i < framesRead * channels; ++i) {
        output[i] *= volume;
    }

    // Feed mono samples to the spectrum analyzer running on its own thread.
    if (impl->spectrumEnabled && impl->analyzer) {
        if (impl->spectrumMonoBuffer.size() < framesRead)
            impl->spectrumMonoBuffer.resize(framesRead);
        for (ma_uint64 i = 0; i < framesRead; ++i) {
            float sum = 0.0f;
            for (ma_uint32 ch = 0; ch < channels; ++ch) {
                sum += output[i * channels + ch];
            }
            impl->spectrumMonoBuffer[i] = sum / static_cast<float>(channels);
        }
        impl->analyzer->feedSamples(impl->spectrumMonoBuffer.data(), framesRead);
    }

    // EOF detection: zero remaining frames and signal the main thread.
    if (framesRead < frameCount) {
        void *silenceStart = static_cast<ma_uint8 *>(pOutput) + framesRead * channels * sizeof(float);
        ma_silence_pcm_frames(silenceStart, frameCount - framesRead, ma_format_f32, channels);
        impl->finishedFlag.store(true);
    }
}

bool AudioEngine::Impl::initDevice()
{
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = decoder.outputFormat;
    config.playback.channels = decoder.outputChannels;
    config.sampleRate = decoder.outputSampleRate;
    config.dataCallback = dataCallback;
    config.pUserData = this;

    return ma_device_init(nullptr, &config, &device) == MA_SUCCESS;
}

void AudioEngine::Impl::updateState(PlaybackState newState)
{
    if (state == newState)
        return;
    state = newState;
    emit q->playbackStateChanged(state);
}

void AudioEngine::Impl::close()
{
    if (positionTimer)
        positionTimer->stop();

    if (analyzer)
        analyzer->stop();

    ma_device_stop(&device);
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);

    finishedFlag = false;
}

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->q = this;
    d->analyzer = std::make_unique<SpectrumAnalyzer>(this);

    d->positionTimer = new QTimer(this);
    d->positionTimer->setInterval(d->positionIntervalMs);

    connect(d->positionTimer, &QTimer::timeout, this, [this]() {
        if (d->state != PlaybackState::Playing)
            return;

        ma_uint64 cursor = 0;
        if (ma_decoder_get_cursor_in_pcm_frames(&d->decoder, &cursor) == MA_SUCCESS) {
            qint64 posMs = static_cast<qint64>(cursor * 1000.0 / d->decoder.outputSampleRate);
            emit positionChanged(posMs);
        }

        // finishedFlag is written by the audio thread; read-and-clear from the main thread.
        if (d->finishedFlag.exchange(false)) {
            emit finished();
            stop();
        }
    });
}

AudioEngine::~AudioEngine()
{
    stop();
}

void AudioEngine::play(const QString &filePath)
{
    stop();

    d->currentPath = filePath;
    d->finishedFlag = false;

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
#ifdef Q_OS_WIN
    if (ma_decoder_init_file_w(reinterpret_cast<const wchar_t*>(filePath.utf16()), &decoderConfig, &d->decoder) != MA_SUCCESS) {
#else
    if (ma_decoder_init_file(filePath.toUtf8().constData(), &decoderConfig, &d->decoder) != MA_SUCCESS) {
#endif
        qWarning() << "AudioEngine: failed to initialize decoder for" << filePath;
        emit error(tr("Failed to decode file: %1").arg(filePath));
        return;
    }

    ma_uint64 length = 0;
    if (ma_decoder_get_length_in_pcm_frames(&d->decoder, &length) == MA_SUCCESS) {
        d->durationMs = static_cast<qint64>(length * 1000.0 / d->decoder.outputSampleRate);
    } else {
        d->durationMs = 0;
    }
    emit durationChanged(d->durationMs);

    if (!d->initDevice()) {
        ma_decoder_uninit(&d->decoder);
        emit error(tr("Failed to initialize audio device"));
        return;
    }

    if (d->spectrumEnabled) {
        d->analyzer->start(d->decoder.outputSampleRate);
    }

    if (ma_device_start(&d->device) == MA_SUCCESS) {
        d->updateState(PlaybackState::Playing);
        d->positionTimer->start();
    } else {
        d->close();
        emit error(tr("Failed to start playback"));
    }
}

void AudioEngine::pause()
{
    if (d->state != PlaybackState::Playing)
        return;

    ma_device_stop(&d->device);
    d->updateState(PlaybackState::Paused);
    d->positionTimer->stop();
}

void AudioEngine::resume()
{
    if (d->state != PlaybackState::Paused)
        return;

    if (ma_device_start(&d->device) == MA_SUCCESS) {
        d->updateState(PlaybackState::Playing);
        d->positionTimer->start();
    }
}

void AudioEngine::stop()
{
    if (d->state == PlaybackState::Stopped)
        return;

    d->close();
    d->updateState(PlaybackState::Stopped);
    emit positionChanged(0);
}

void AudioEngine::seek(qint64 ms)
{
    if (d->state == PlaybackState::Stopped)
        return;

    d->finishedFlag = false;

    // Decoder is not thread-safe; stop the device while seeking to avoid racing
    // the audio callback, then restart if we were playing.
    bool wasPlaying = (d->state == PlaybackState::Playing);
    if (wasPlaying) {
        ma_device_stop(&d->device);
    }

    ma_uint64 frame = static_cast<ma_uint64>(ms * d->decoder.outputSampleRate / 1000.0);
    ma_decoder_seek_to_pcm_frame(&d->decoder, frame);

    if (wasPlaying) {
        ma_device_start(&d->device);
    }

    emit positionChanged(ms);
}

AudioEngine::PlaybackState AudioEngine::playbackState() const
{
    return d->state;
}

qint64 AudioEngine::position() const
{
    if (d->state == PlaybackState::Stopped)
        return 0;

    ma_uint64 cursor = 0;
    if (ma_decoder_get_cursor_in_pcm_frames(&d->decoder, &cursor) == MA_SUCCESS) {
        return static_cast<qint64>(cursor * 1000.0 / d->decoder.outputSampleRate);
    }
    return 0;
}

qint64 AudioEngine::duration() const
{
    return d->durationMs;
}

float AudioEngine::volume() const
{
    return d->volume.load();
}

bool AudioEngine::spectrumEnabled() const
{
    return d->spectrumEnabled.load();
}

int AudioEngine::positionUpdateInterval() const
{
    return d->positionIntervalMs;
}

void AudioEngine::setVolume(float volume)
{
    volume = std::clamp(volume, 0.0f, 1.0f);
    if (qFuzzyCompare(d->volume.load(), volume))
        return;

    d->volume.store(volume);
    emit volumeChanged(volume);
}

void AudioEngine::setSpectrumEnabled(bool enabled)
{
    if (d->spectrumEnabled.load() == enabled)
        return;

    d->spectrumEnabled.store(enabled);
    emit spectrumEnabledChanged(enabled);
}

void AudioEngine::setPositionUpdateInterval(int ms)
{
    ms = (std::max)(50, ms);
    if (d->positionIntervalMs == ms)
        return;

    d->positionIntervalMs = ms;
    if (d->positionTimer)
        d->positionTimer->setInterval(ms);

    emit positionUpdateIntervalChanged(ms);
}

} // namespace Velvox

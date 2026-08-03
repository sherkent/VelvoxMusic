#pragma once

#include <QObject>
#include <QVector>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

// kissfft complex type; keep header lightweight by forward-declaring would
// require a wrapper, so include the small header directly.
#include "kiss_fft.h"

namespace Velvox {

/**
 * @brief Real-time spectrum analyzer.
 *
 * PCM samples are pushed from the audio callback thread into a lock-free
 * ring buffer. A dedicated worker thread reads blocks, applies a Hann window,
 * runs a real FFT (kissfft), and emits logarithmic band energies back to the
 * UI thread via Qt::QueuedConnection.
 */
class SpectrumAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit SpectrumAnalyzer(QObject *parent = nullptr);
    ~SpectrumAnalyzer() override;

    void start(int sampleRate);
    void stop();
    void feedSamples(const float *samples, size_t count);

signals:
    void spectrumChanged(const QVector<float> &bands);

private:
    void processLoop();

    class SpscRingBuffer;

    static constexpr size_t kRingCapacity = 8192;
    static constexpr size_t kFftSize = 1024;
    static constexpr size_t kNumBands = 32;

    std::unique_ptr<SpscRingBuffer> m_ringBuffer;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    int m_sampleRate = 44100;

    void *m_fftConfig = nullptr;
    std::vector<float> m_fftIn;
    std::vector<float> m_fftOutReal;
    std::vector<float> m_fftOutImag;
    std::vector<float> m_window;

    // Re-used per-iteration buffers to avoid heap allocation in the analyzer loop.
    std::vector<float> m_samples;
    QVector<float> m_bands;
    std::vector<kiss_fft_cpx> m_fftOut;
};

} // namespace Velvox

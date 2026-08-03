#include "core/SpectrumAnalyzer.h"

#include "kiss_fft.h"
#include "kiss_fftr.h"

#include <QDebug>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Velvox {

/**
 * @brief Single-producer single-consumer lock-free ring buffer.
 *
 * The producer is miniaudio's high-priority audio callback thread.
 * The consumer is the analyzer worker thread.
 */
class SpectrumAnalyzer::SpscRingBuffer
{
public:
    explicit SpscRingBuffer(size_t capacity)
        : m_buffer(capacity)
        , m_capacity(capacity)
    {
    }

    bool write(const float *data, size_t count)
    {
        size_t writeIndex = m_writeIndex.load(std::memory_order_relaxed);
        size_t readIndex = m_readIndex.load(std::memory_order_acquire);
        size_t available = m_capacity - (writeIndex - readIndex);

        // Drop oldest samples if the consumer is too slow.
        if (count > available) {
            size_t toDrop = count - available;
            m_readIndex.store(readIndex + toDrop, std::memory_order_release);
            readIndex += toDrop;
        }

        for (size_t i = 0; i < count; ++i) {
            m_buffer[(writeIndex + i) % m_capacity] = data[i];
        }
        m_writeIndex.store(writeIndex + count, std::memory_order_release);
        return true;
    }

    size_t read(float *data, size_t maxCount)
    {
        size_t readIndex = m_readIndex.load(std::memory_order_relaxed);
        size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);
        size_t available = writeIndex - readIndex;
        size_t toRead = std::min(available, maxCount);

        for (size_t i = 0; i < toRead; ++i) {
            data[i] = m_buffer[(readIndex + i) % m_capacity];
        }
        m_readIndex.store(readIndex + toRead, std::memory_order_release);
        return toRead;
    }

private:
    std::vector<float> m_buffer;
    size_t m_capacity;
    std::atomic<size_t> m_writeIndex{0};
    std::atomic<size_t> m_readIndex{0};
};

SpectrumAnalyzer::SpectrumAnalyzer(QObject *parent)
    : QObject(parent)
    , m_ringBuffer(std::make_unique<SpscRingBuffer>(kRingCapacity))
    , m_fftConfig(kiss_fftr_alloc(kFftSize, 0, nullptr, nullptr))
    , m_fftIn(kFftSize)
    , m_fftOutReal(kFftSize / 2 + 1)
    , m_fftOutImag(kFftSize / 2 + 1)
    , m_window(kFftSize)
    , m_samples(kFftSize)
    , m_bands(static_cast<int>(kNumBands))
    , m_fftOut(kFftSize / 2 + 1)
{
    if (!m_fftConfig) {
        qWarning() << "Failed to allocate kissfft config";
        return;
    }

    for (size_t i = 0; i < kFftSize; ++i) {
        m_window[i] = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * i / (kFftSize - 1));
    }
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    stop();
    kiss_fftr_free(static_cast<kiss_fftr_cfg>(m_fftConfig));
}

void SpectrumAnalyzer::start(int sampleRate)
{
    stop();
    m_sampleRate = sampleRate;
    m_running = true;
    m_thread = std::thread(&SpectrumAnalyzer::processLoop, this);
}

void SpectrumAnalyzer::stop()
{
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void SpectrumAnalyzer::feedSamples(const float *samples, size_t count)
{
    m_ringBuffer->write(samples, count);
}

void SpectrumAnalyzer::processLoop()
{
    while (m_running) {
        size_t got = m_ringBuffer->read(m_samples.data(), kFftSize);
        if (got < kFftSize) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        // Apply Hann window and run real FFT.
        for (size_t i = 0; i < kFftSize; ++i) {
            m_fftIn[i] = m_samples[i] * m_window[i];
        }

        kiss_fftr(static_cast<kiss_fftr_cfg>(m_fftConfig), m_fftIn.data(), m_fftOut.data());

        // Convert complex output to split arrays for easier reading.
        for (size_t i = 0; i < m_fftOut.size(); ++i) {
            m_fftOutReal[i] = m_fftOut[i].r;
            m_fftOutImag[i] = m_fftOut[i].i;
        }

        // Compute logarithmic band energies.
        const float maxFreq = m_sampleRate / 2.0f;
        const int binCount = static_cast<int>(kFftSize / 2 + 1);
        std::fill(m_bands.begin(), m_bands.end(), 0.0f);

        for (size_t b = 0; b < kNumBands; ++b) {
            float tLow = static_cast<float>(b) / kNumBands;
            float tHigh = static_cast<float>(b + 1) / kNumBands;

            // Log-spaced bands from ~20 Hz to Nyquist.
            float fLow = maxFreq * std::pow(10.0f, -3.0f + 3.0f * tLow);
            float fHigh = maxFreq * std::pow(10.0f, -3.0f + 3.0f * tHigh);

            int binLow = std::clamp(static_cast<int>(fLow / maxFreq * binCount), 0, binCount - 1);
            int binHigh = std::clamp(static_cast<int>(fHigh / maxFreq * binCount), binLow, binCount);

            float energy = 0.0f;
            for (int bin = binLow; bin < binHigh; ++bin) {
                float real = m_fftOutReal[bin];
                float imag = m_fftOutImag[bin];
                energy += std::sqrt(real * real + imag * imag);
            }
            energy /= std::max(1, binHigh - binLow);

            // Empirical scaling; tune later against real music.
            m_bands[static_cast<int>(b)] = std::clamp(energy * 5.0f, 0.0f, 1.0f);
        }

        // Emitted from the analyzer thread; Qt will queue it to the QML thread.
        emit spectrumChanged(m_bands);
    }
}

} // namespace Velvox

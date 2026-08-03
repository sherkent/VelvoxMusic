#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

namespace Velvox {

class AudioEngine : public QObject
{
    Q_OBJECT
    Q_ENUM(PlaybackState)

public:
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };

    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine() override;

    Q_INVOKABLE void play(const QString &filePath);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 ms);

    PlaybackState playbackState() const;
    qint64 position() const;
    qint64 duration() const;
    float volume() const;
    bool spectrumEnabled() const;
    int positionUpdateInterval() const;

public slots:
    void setVolume(float volume);
    void setSpectrumEnabled(bool enabled);
    void setPositionUpdateInterval(int ms);

signals:
    void playbackStateChanged(PlaybackState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void volumeChanged(float volume);
    void spectrumEnabledChanged(bool enabled);
    void positionUpdateIntervalChanged(int interval);
    void finished();
    void spectrumChanged(const QVector<float> &bands);
    void error(const QString &message);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Velvox

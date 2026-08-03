#pragma once

#include "core/DatabaseManager.h"
#include "core/MetadataReader.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <atomic>

namespace Velvox {

class LibraryScannerWorker;

/**
 * @brief Manages the local music library.
 *
 * - Keeps a list of music directories.
 * - Runs an asynchronous recursive scan on a background thread.
 * - Reports progress so the UI can show "Scanned X/Y".
 * - Cleans up DB entries for files that no longer exist.
 *
 * Thread safety: the worker creates its own DatabaseManager / SQLite connection
 * inside the background thread so that QSqlDatabase is not used across threads.
 */
class LibraryManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)

public:
    explicit LibraryManager(DatabaseManager *database,
                            MetadataReader *metadataReader,
                            QObject *parent = nullptr);
    ~LibraryManager() override;

    Q_INVOKABLE void addMusicDirectory(const QString &path);
    Q_INVOKABLE void removeMusicDirectory(const QString &path);
    Q_INVOKABLE void clearMusicDirectories();

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void cancelScan();

    QStringList musicDirectories() const;
    bool isScanning() const;

signals:
    void scanStarted();
    void scanProgress(int processed, int total, const QString &currentFile);
    void scanFinished(int added, int updated, int removed);
    void scanCancelled();
    void scanError(const QString &filePath, const QString &error);
    void musicDirectoriesChanged();
    void scanningChanged();

private:
    void onWorkerFinished(int added, int updated, int removed);

    DatabaseManager *m_db = nullptr;
    MetadataReader *m_reader = nullptr;

    QStringList m_directories;
    std::atomic<bool> m_scanning{false};
    std::atomic<bool> m_cancelled{false};

    QThread *m_scanThread = nullptr;
    LibraryScannerWorker *m_scanWorker = nullptr;
};

/**
 * @brief Background worker that discovers audio files and imports metadata.
 */
class LibraryScannerWorker : public QObject
{
    Q_OBJECT

public:
    explicit LibraryScannerWorker(const QStringList &directories,
                                  const QString &dbPath,
                                  MetadataReader *reader,
                                  std::atomic<bool> *cancelled,
                                  QObject *parent = nullptr);

signals:
    void progress(int processed, int total, const QString &currentFile);
    void finished(int added, int updated, int removed);
    void error(const QString &filePath, const QString &message);

public slots:
    void scan();

private:
    QStringList m_directories;
    QString m_dbPath;
    MetadataReader *m_reader = nullptr;
    std::atomic<bool> *m_cancelled = nullptr;
};

} // namespace Velvox

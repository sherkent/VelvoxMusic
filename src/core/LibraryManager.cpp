#include "core/LibraryManager.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

namespace Velvox {

namespace {

const QStringList kAudioExtensions = {
    QStringLiteral("*.mp3"),
    QStringLiteral("*.flac"),
    QStringLiteral("*.wav"),
    QStringLiteral("*.ogg"),
    QStringLiteral("*.m4a"),
    QStringLiteral("*.aac"),
    QStringLiteral("*.wma")
};

} // anonymous namespace

LibraryScannerWorker::LibraryScannerWorker(const QStringList &directories,
                                           const QString &dbPath,
                                           MetadataReader *reader,
                                           std::atomic<bool> *cancelled,
                                           QObject *parent)
    : QObject(parent)
    , m_directories(directories)
    , m_dbPath(dbPath)
    , m_reader(reader)
    , m_cancelled(cancelled)
{
}

void LibraryScannerWorker::scan()
{
    int added = 0;
    int updated = 0;

    // Open a dedicated DB connection inside this thread. QSqlDatabase must not
    // be shared across threads. Use a unique connection name per worker.
    DatabaseManager db(m_dbPath, QStringLiteral("library-worker-%1").arg(quintptr(this)));
    if (!db.initialize()) {
        emit finished(0, 0, 0);
        return;
    }

    const int oldTrackCount = db.trackCount();

    // 1. Discover audio files.
    QStringList files;
    for (const QString &dir : m_directories) {
        if (m_cancelled->load())
            break;

        QDirIterator it(dir, kAudioExtensions, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (m_cancelled->load())
                break;
            files.append(it.next());
        }
    }

    const int total = files.size();

    // 2. Read metadata and upsert into DB.
    for (int i = 0; i < files.size(); ++i) {
        if (m_cancelled->load())
            break;

        const QString &path = files.at(i);
        emit progress(i + 1, total, QFileInfo(path).fileName());

        Track existing = db.getTrackByPath(path);
        Track track = m_reader->readTrack(path);

        if (!existing.isValid()) {
            if (db.addOrUpdateTrack(track))
                ++added;
            else
                emit error(path, tr("Failed to insert track into database"));
        } else if (existing.fileMtime != track.fileMtime) {
            // Preserve DB-only fields when updating.
            track.id = existing.id;
            track.dateAdded = existing.dateAdded;
            track.playCount = existing.playCount;
            if (db.addOrUpdateTrack(track))
                ++updated;
            else
                emit error(path, tr("Failed to update track in database"));
        }
    }

    // 3. Remove entries for files that are no longer in the library folders.
    int removed = 0;
    if (!m_cancelled->load()) {
        if (db.removeTracksNotIn(files)) {
            const int newTrackCount = db.trackCount();
            removed = qMax(0, oldTrackCount + added - newTrackCount);
        }
    }

    emit finished(added, updated, removed);
}

LibraryManager::LibraryManager(DatabaseManager *database,
                               MetadataReader *metadataReader,
                               QObject *parent)
    : QObject(parent)
    , m_db(database)
    , m_reader(metadataReader)
{
}

LibraryManager::~LibraryManager()
{
    cancelScan();
}

void LibraryManager::addMusicDirectory(const QString &path)
{
    QString canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty())
        canonical = QDir::cleanPath(path);

    if (!m_directories.contains(canonical)) {
        m_directories.append(canonical);
        emit musicDirectoriesChanged();
    }
}

void LibraryManager::removeMusicDirectory(const QString &path)
{
    QString canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty())
        canonical = QDir::cleanPath(path);

    if (m_directories.removeOne(canonical)) {
        emit musicDirectoriesChanged();
    }
}

void LibraryManager::clearMusicDirectories()
{
    if (!m_directories.isEmpty()) {
        m_directories.clear();
        emit musicDirectoriesChanged();
    }
}

QStringList LibraryManager::musicDirectories() const
{
    return m_directories;
}

bool LibraryManager::isScanning() const
{
    return m_scanning.load();
}

void LibraryManager::startScan()
{
    if (m_directories.isEmpty() || m_scanning.load())
        return;

    cancelScan(); // Clean up any previous thread.

    m_cancelled = false;
    m_scanning = true;
    emit scanningChanged();

    m_scanThread = new QThread(this);
    m_scanWorker = new LibraryScannerWorker(m_directories, m_db->dbPath(), m_reader, &m_cancelled);
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanThread, &QThread::started, m_scanWorker, &LibraryScannerWorker::scan);
    connect(m_scanWorker, &LibraryScannerWorker::progress, this, &LibraryManager::scanProgress);
    connect(m_scanWorker, &LibraryScannerWorker::error, this, &LibraryManager::scanError);
    connect(m_scanWorker, &LibraryScannerWorker::finished, this, &LibraryManager::onWorkerFinished);

    // Cleanup after finish.
    connect(m_scanWorker, &LibraryScannerWorker::finished, m_scanThread, &QThread::quit);
    connect(m_scanWorker, &LibraryScannerWorker::finished, m_scanWorker, &QObject::deleteLater);
    connect(m_scanThread, &QThread::finished, m_scanThread, &QObject::deleteLater);

    m_scanThread->start();
    emit scanStarted();
}

void LibraryManager::cancelScan()
{
    m_cancelled = true;

    if (m_scanThread && m_scanThread->isRunning()) {
        m_scanThread->quit();
        m_scanThread->wait(3000);
    }

    if (m_scanning.load()) {
        m_scanning = false;
        emit scanningChanged();
        emit scanCancelled();
    }
}

void LibraryManager::onWorkerFinished(int added, int updated, int removed)
{
    m_scanning = false;
    emit scanningChanged();
    emit scanFinished(added, updated, removed);
}

} // namespace Velvox

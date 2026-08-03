#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QDir>
#include <QSystemTrayIcon>
#include <QMenu>
#include <qqml.h>

#include "core/AudioEngine.h"
#include "core/DatabaseManager.h"
#include "core/LibraryManager.h"
#include "core/MetadataReader.h"
#include "core/LyricsParser.h"
#include "core/WindowsSystemIntegration.h"
#include "models/TrackListModel.h"
#include "models/TrackListProxyModel.h"
#include "models/AlbumListModel.h"
#include "models/ArtistListModel.h"
#include "models/PlaylistModel.h"
#include "models/LyricsModel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Velvox");
    app.setApplicationName("Velvox Music");

    // Local application data directory. On Windows this resolves to something
    // like C:\Users\<user>\AppData\Local\Velvox\Velvox Music.
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (appDataDir.isEmpty())
        appDataDir = QDir::currentPath() + "/data";
    QDir(appDataDir).mkpath(".");

    const QString dbPath = appDataDir + "/library.db";
    const QString coverCacheDir = appDataDir + "/covers";

    Velvox::AudioEngine audioEngine;
    audioEngine.setPositionUpdateInterval(200);
    audioEngine.setSpectrumEnabled(true);

    Velvox::DatabaseManager dbManager(dbPath);
    if (!dbManager.initialize()) {
        qWarning() << "Failed to initialize library database at" << dbPath;
    }

    Velvox::MetadataReader metadataReader(coverCacheDir);
    Velvox::LibraryManager libraryManager(&dbManager, &metadataReader);
    Velvox::TrackListModel trackListSource(&dbManager);
    Velvox::TrackListProxyModel trackListModel(&trackListSource);
    Velvox::AlbumListModel albumListModel(&dbManager);
    Velvox::ArtistListModel artistListModel(&dbManager);
    Velvox::PlaylistModel playlistModel(&dbManager);
    Velvox::LyricsModel lyricsModel;
    Velvox::WindowsSystemIntegration windowsIntegration;

    // Connect scan completion to model reload so the QML list stays current.
    QObject::connect(&libraryManager, &Velvox::LibraryManager::scanFinished,
                     &trackListSource, &Velvox::TrackListModel::reload);
    QObject::connect(&libraryManager, &Velvox::LibraryManager::scanCancelled,
                     &trackListSource, &Velvox::TrackListModel::reload);
    QObject::connect(&libraryManager, &Velvox::LibraryManager::scanFinished,
                     &albumListModel, &Velvox::AlbumListModel::reload);
    QObject::connect(&libraryManager, &Velvox::LibraryManager::scanCancelled,
                     &albumListModel, &Velvox::AlbumListModel::reload);
    QObject::connect(&libraryManager, &Velvox::LibraryManager::scanFinished,
                     &artistListModel, &Velvox::ArtistListModel::reload);
    QObject::connect(&libraryManager, &Velvox::LibraryManager::scanCancelled,
                     &artistListModel, &Velvox::ArtistListModel::reload);

    // Lyrics follow the current track.
    QObject::connect(&audioEngine, &Velvox::AudioEngine::positionChanged,
                     &lyricsModel, &Velvox::LyricsModel::setPosition);

    // Register types for QML.
    qmlRegisterUncreatableType<Velvox::AudioEngine>(
        "VelvoxMusic", 1, 0, "AudioEngine",
        "AudioEngine is created from C++ and exposed as the audioEngine context property");
    qmlRegisterUncreatableType<Velvox::LibraryManager>(
        "VelvoxMusic", 1, 0, "LibraryManager",
        "LibraryManager is created from C++ and exposed as the libraryManager context property");
    qmlRegisterUncreatableType<Velvox::TrackListModel>(
        "VelvoxMusic", 1, 0, "TrackListModel",
        "TrackListModel exposes track role enums; the trackListModel context property is a TrackListProxyModel");
    qmlRegisterUncreatableType<Velvox::TrackListProxyModel>(
        "VelvoxMusic", 1, 0, "TrackListProxyModel",
        "TrackListProxyModel is created from C++ and exposed as the trackListModel context property");
    qmlRegisterUncreatableType<Velvox::AlbumListModel>(
        "VelvoxMusic", 1, 0, "AlbumListModel",
        "AlbumListModel is created from C++ and exposed as the albumListModel context property");
    qmlRegisterUncreatableType<Velvox::ArtistListModel>(
        "VelvoxMusic", 1, 0, "ArtistListModel",
        "ArtistListModel is created from C++ and exposed as the artistListModel context property");
    qmlRegisterUncreatableType<Velvox::PlaylistModel>(
        "VelvoxMusic", 1, 0, "PlaylistModel",
        "PlaylistModel is created from C++ and exposed as the playlistModel context property");
    qmlRegisterUncreatableType<Velvox::LyricsModel>(
        "VelvoxMusic", 1, 0, "LyricsModel",
        "LyricsModel is created from C++ and exposed as the lyricsModel context property");
    qmlRegisterUncreatableType<Velvox::WindowsSystemIntegration>(
        "VelvoxMusic", 1, 0, "WindowsSystemIntegration",
        "WindowsSystemIntegration is created from C++ and exposed as the windowsIntegration context property");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("audioEngine", &audioEngine);
    engine.rootContext()->setContextProperty("libraryManager", &libraryManager);
    engine.rootContext()->setContextProperty("trackListModel", &trackListModel);
    engine.rootContext()->setContextProperty("albumListModel", &albumListModel);
    engine.rootContext()->setContextProperty("artistListModel", &artistListModel);
    engine.rootContext()->setContextProperty("playlistModel", &playlistModel);
    engine.rootContext()->setContextProperty("lyricsModel", &lyricsModel);
    engine.rootContext()->setContextProperty("windowsIntegration", &windowsIntegration);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("VelvoxMusic", "main");

    // System tray icon. Requires QApplication (not QGuiApplication).
    QSystemTrayIcon trayIcon;
    QMenu trayMenu;
    QAction playPauseAction(&trayMenu);
    QAction prevAction(&trayMenu);
    QAction nextAction(&trayMenu);
    QAction quitAction(&trayMenu);

    auto updatePlayPauseText = [&]() {
        const bool playing = audioEngine.playbackState() == Velvox::AudioEngine::PlaybackState::Playing;
        playPauseAction.setText(playing ? QObject::tr("Pause") : QObject::tr("Play"));
    };

    playPauseAction.setText(QObject::tr("Play"));
    prevAction.setText(QObject::tr("Previous"));
    nextAction.setText(QObject::tr("Next"));
    quitAction.setText(QObject::tr("Quit"));

    QObject::connect(&playPauseAction, &QAction::triggered, &windowsIntegration, [&]() {
        emit windowsIntegration.mediaPlayPauseRequested();
    });
    QObject::connect(&prevAction, &QAction::triggered, &windowsIntegration, [&]() {
        emit windowsIntegration.mediaPreviousRequested();
    });
    QObject::connect(&nextAction, &QAction::triggered, &windowsIntegration, [&]() {
        emit windowsIntegration.mediaNextRequested();
    });
    QObject::connect(&quitAction, &QAction::triggered, &app, &QApplication::quit);

    trayMenu.addAction(&playPauseAction);
    trayMenu.addAction(&prevAction);
    trayMenu.addAction(&nextAction);
    trayMenu.addSeparator();
    trayMenu.addAction(&quitAction);

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.setToolTip(QObject::tr("Velvox Music"));
    trayIcon.setIcon(QIcon(QStringLiteral(":/qt/qml/VelvoxMusic/resources/icons/app-icon.svg")));
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, &engine, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            for (QObject *obj : engine.rootObjects()) {
                if (QWindow *window = qobject_cast<QWindow *>(obj)) {
                    window->showNormal();
                    window->raise();
                    window->requestActivate();
                    break;
                }
            }
        }
    });
    trayIcon.show();

    QObject::connect(&audioEngine, &Velvox::AudioEngine::playbackStateChanged,
                     &trayIcon, updatePlayPauseText);

    // Windows-specific media keys and taskbar thumbnail buttons.
    windowsIntegration.install(&app, &audioEngine);

    return app.exec();
}

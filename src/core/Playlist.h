#pragma once

#include <QString>
#include <QDateTime>

namespace Velvox {

struct Playlist
{
    int id = -1;
    QString name;
    QDateTime dateCreated;
    QDateTime dateModified;

    bool isValid() const { return id >= 0; }
};

} // namespace Velvox

#include "LibraryModel.h"
#include <QFileInfo>

LibraryModel::LibraryModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

void LibraryModel::setTracks(const QList<TrackMetadata>& tracks) {
    beginResetModel();
    tracks_ = tracks;
    endResetModel();
}

TrackMetadata LibraryModel::trackAt(int row) const {
    if (row < 0 || row >= tracks_.size()) return {};
    return tracks_.at(row);
}

int LibraryModel::rowForId(int trackId) const {
    for (int i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i].id == trackId) return i;
    }
    return -1;
}

int LibraryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : tracks_.size();
}

int LibraryModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= tracks_.size())
        return {};

    const auto& t = tracks_.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColIndex:       return t.trackNumber > 0 ? t.trackNumber : QVariant();
        case ColTitle:       return t.displayTitle();
        case ColArtist:      return t.displayArtist();
        case ColAlbum:       return t.album;
        case ColYear:        return t.year > 0 ? t.year : QVariant();
        case ColDuration:    return t.formattedDuration();
        case ColFormat:      return t.format;
        case ColSampleRate:  return QStringLiteral("%1 kHz").arg(t.sampleRate / 1000.0, 0, 'f', 1);
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColIndex || index.column() == ColYear ||
            index.column() == ColDuration || index.column() == ColSampleRate)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    if (role == Qt::ToolTipRole) {
        return t.path;
    }

    if (role == Qt::UserRole) {
        return t.id;
    }

    if (role == CoverArtHashRole) {
        return t.coverArtHash;
    }

    return {};
}

QVariant LibraryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColIndex:       return QStringLiteral("#");
    case ColTitle:       return QStringLiteral("Title");
    case ColArtist:      return QStringLiteral("Artist");
    case ColAlbum:       return QStringLiteral("Album");
    case ColYear:        return QStringLiteral("Year");
    case ColDuration:    return QStringLiteral("Duration");
    case ColFormat:      return QStringLiteral("Format");
    case ColSampleRate:  return QStringLiteral("Sample Rate");
    }
    return {};
}

QModelIndex LibraryModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() || row < 0 || row >= tracks_.size())
        return {};
    return createIndex(row, column);
}

QModelIndex LibraryModel::parent(const QModelIndex&) const {
    return {};
}

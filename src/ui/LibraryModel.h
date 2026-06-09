#pragma once

#include <QAbstractItemModel>
#include <QList>
#include "library/TrackMetadata.h"

class LibraryModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum Column {
        ColIndex = 0,
        ColTitle,
        ColArtist,
        ColAlbum,
        ColYear,
        ColDuration,
        ColFormat,
        ColSampleRate,
        ColumnCount
    };

    explicit LibraryModel(QObject* parent = nullptr);

    void setTracks(const QList<TrackMetadata>& tracks);

    TrackMetadata trackAt(int row) const;
    int rowForId(int trackId) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    static constexpr int CoverArtHashRole = Qt::UserRole + 1;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex&) const override;

private:
    QList<TrackMetadata> tracks_;
};

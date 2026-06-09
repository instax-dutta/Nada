#pragma once

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include <QLineEdit>
#include <QToolButton>
#include <QComboBox>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>
#include "LibraryModel.h"
#include "library/TrackMetadata.h"

class AlbumDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit AlbumDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override;
};

class LibraryView : public QWidget {
    Q_OBJECT
public:
    explicit LibraryView(QWidget* parent = nullptr);

    void setModel(LibraryModel* model);
    LibraryModel* model() const { return model_; }
    QList<TrackMetadata> selectedTracks() const;

signals:
    void trackSelected(const TrackMetadata& track);
    void trackActivated(const TrackMetadata& track);
    void albumSelected(const QString& album, const QString& artist);

private slots:
    void onSearchTextChanged(const QString& text);
    void onViewToggled();
    void onAlbumClicked(const QModelIndex& index);

private:
    void setupToolbar();
    void setupList();
    void setupGrid();
    void restoreColumnWidths();

    LibraryModel* model_ = nullptr;
    QSortFilterProxyModel* proxy_ = nullptr;

    QLineEdit* searchBar_ = nullptr;
    QToolButton* listViewBtn_ = nullptr;
    QToolButton* gridViewBtn_ = nullptr;
    QComboBox* sortCombo_ = nullptr;

    QTreeView* treeView_ = nullptr;
    QListView* gridView_ = nullptr;
    AlbumDelegate* albumDelegate_ = nullptr;

    QSplitter* splitter_ = nullptr;

    bool gridMode_ = false;
    QWidget* toolbar_ = nullptr;
};

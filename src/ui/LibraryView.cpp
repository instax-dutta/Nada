#include "LibraryView.h"
#include "Theme.h"
#include "library/CoverArtCache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QTimer>
#include <QApplication>
#include <QPixmap>
#include <QAbstractItemView>

AlbumDelegate::AlbumDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void AlbumDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRect r = option.rect;
    bool hovered = option.state & QStyle::State_MouseOver;
    bool selected = option.state & QStyle::State_Selected;

    QColor bg = selected ? Theme::ELEVATED : (hovered ? QColor(0x33, 0x33, 0x33) : Theme::SURFACE);
    painter->fillRect(r, bg);

    int imgSize = 160;
    int x = r.x() + (r.width() - imgSize) / 2;
    int y = r.y() + 8;

    QString album = index.data(Qt::DisplayRole).toString();
    QString artist = index.data(Qt::UserRole).toString();

    CoverArtCache cache;
    QPixmap cover = cache.get(album + artist);
    if (cover.isNull()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x44, 0x44, 0x44));
        painter->drawRoundedRect(x, y, imgSize, imgSize, 2, 2);
    } else {
        painter->drawPixmap(x, y, imgSize, imgSize, cover);
    }

    if (hovered && !selected) {
        painter->fillRect(x, y, imgSize, imgSize, QColor(0, 0, 0, 60));
    }

    painter->setPen(Theme::TEXT_PRI);
    QFont f = Theme::uiFont();
    f.setPointSize(11);
    painter->setFont(f);
    QRect titleRect(r.x(), y + imgSize + 6, r.width(), 18);
    painter->drawText(titleRect, Qt::AlignHCenter | Qt::AlignTop | Qt::ElideRight, album);

    painter->setPen(Theme::TEXT_SEC);
    QRect artistRect(r.x(), y + imgSize + 22, r.width(), 16);
    painter->drawText(artistRect, Qt::AlignHCenter | Qt::AlignTop | Qt::ElideRight, artist);

    painter->restore();
}

QSize AlbumDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return QSize(180, 200);
}

class TrackDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRect r = option.rect;
        bool hovered = option.state & QStyle::State_MouseOver;
        bool selected = option.state & QStyle::State_Selected;

        QColor bg = selected ? Theme::ACCENT_DIM : (hovered ? Theme::CARD : Qt::transparent);
        if (selected || hovered)
            painter->fillRect(r, bg);

        painter->setPen(Theme::BORDER);
        painter->drawLine(r.bottomLeft(), r.bottomRight());

        int col = index.column();
        QVariant displayData = index.data(Qt::DisplayRole);

        auto secColor = selected && col == LibraryModel::ColTitle ? Theme::ACCENT : Theme::TEXT_SEC;

        if (col == LibraryModel::ColIndex && displayData.isValid()) {
            painter->setFont(Theme::monoFont());
            painter->setPen(Theme::TEXT_SEC);
            painter->drawText(r.adjusted(4, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, displayData.toString());
        } else if (col == LibraryModel::ColDuration && displayData.isValid()) {
            painter->setFont(Theme::monoFont());
            painter->setPen(Theme::TEXT_SEC);
            painter->drawText(r.adjusted(4, 0, -8, 0), Qt::AlignRight | Qt::AlignVCenter, displayData.toString());
        } else if (col == LibraryModel::ColFormat && displayData.isValid()) {
            painter->setFont(Theme::monoFont());
            painter->setPen(Theme::ACCENT);
            painter->drawText(r.adjusted(4, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, displayData.toString());
        } else if (col == LibraryModel::ColYear && displayData.isValid()) {
            painter->setFont(Theme::uiFont());
            painter->setPen(Theme::TEXT_SEC);
            painter->drawText(r.adjusted(4, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, displayData.toString());
        } else if (col == LibraryModel::ColSampleRate && displayData.isValid()) {
            painter->setFont(Theme::monoFont());
            painter->setPen(Theme::TEXT_SEC);
            painter->drawText(r.adjusted(4, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, displayData.toString());
        } else if (col == LibraryModel::ColTitle) {
            painter->setFont(Theme::uiFont());
            painter->setPen(selected ? Theme::ACCENT : Theme::TEXT_PRI);
            painter->drawText(r.adjusted(36, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, displayData.toString());

            static CoverArtCache coverCache;
            QString coverHash = index.data(LibraryModel::CoverArtHashRole).toString();
            QPixmap cover;
            if (!coverHash.isEmpty())
                cover = coverCache.getThumbnail(coverHash);
            if (!cover.isNull()) {
                painter->drawPixmap(r.x() + 4, r.y() + 3, 32, 32, cover);
            } else {
                painter->setPen(Qt::NoPen);
                painter->setBrush(Theme::CARD);
                painter->drawRoundedRect(r.x() + 4, r.y() + 3, 32, 32, 2, 2);
            }
        } else if (col == LibraryModel::ColArtist && displayData.isValid()) {
            painter->setFont(Theme::uiFont());
            painter->setPen(secColor);
            painter->drawText(r.adjusted(4, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, displayData.toString());
        } else if (col == LibraryModel::ColAlbum && displayData.isValid()) {
            painter->setFont(Theme::uiFont());
            painter->setPen(secColor);
            painter->drawText(r.adjusted(4, 0, -4, 0), Qt::AlignLeft | Qt::AlignVCenter, displayData.toString());
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return QSize(0, 38);
    }
};

LibraryView::LibraryView(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupToolbar();
    layout->addWidget(toolbar_);

    splitter_ = new QSplitter(Qt::Vertical, this);
    splitter_->setHandleWidth(1);
    splitter_->setChildrenCollapsible(true);

    setupGrid();
    setupList();

    splitter_->addWidget(gridView_);
    splitter_->addWidget(treeView_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 2);

    layout->addWidget(splitter_, 1);

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_->setFilterKeyColumn(-1);
}

void LibraryView::setupToolbar() {
    toolbar_ = new QWidget(this);
    toolbar_->setFixedHeight(40);
    toolbar_->setStyleSheet("background-color: #111111; border-bottom: 0.5px solid rgba(30,30,30,0.5);");
    auto* lay = new QHBoxLayout(toolbar_);
    lay->setContentsMargins(8, 4, 8, 4);

    searchBar_ = new QLineEdit(this);
    searchBar_->setPlaceholderText("Search tracks...");
    searchBar_->setClearButtonEnabled(true);
    searchBar_->setFixedWidth(220);
    lay->addWidget(searchBar_);

    auto* searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(200);
    connect(searchBar_, &QLineEdit::textChanged, this, [searchTimer]() {
        searchTimer->start();
    });
    connect(searchTimer, &QTimer::timeout, this, [this]() {
        onSearchTextChanged(searchBar_->text());
    });

    lay->addStretch();

    sortCombo_ = new QComboBox(this);
    sortCombo_->addItems({"Artist", "Album", "Year", "Date Added", "Duration"});
    lay->addWidget(sortCombo_);

    listViewBtn_ = new QToolButton(this);
    listViewBtn_->setText("List");
    listViewBtn_->setCheckable(true);
    listViewBtn_->setChecked(true);
    listViewBtn_->setFixedHeight(26);
    listViewBtn_->setStyleSheet(
        "QToolButton { background: transparent; color: #86868B; border: none; border-radius: 4px; padding: 2px 10px; font-size: 12px; }"
        "QToolButton:checked { color: #D4B982; background-color: #221C12; }"
        "QToolButton:hover:!checked { color: #F5F5F7; background-color: #1E1E1E; }");
    lay->addWidget(listViewBtn_);

    gridViewBtn_ = new QToolButton(this);
    gridViewBtn_->setText("Grid");
    gridViewBtn_->setCheckable(true);
    gridViewBtn_->setFixedHeight(26);
    gridViewBtn_->setStyleSheet(
        "QToolButton { background: transparent; color: #86868B; border: none; border-radius: 4px; padding: 2px 10px; font-size: 12px; }"
        "QToolButton:checked { color: #D4B982; background-color: #221C12; }"
        "QToolButton:hover:!checked { color: #F5F5F7; background-color: #1E1E1E; }");
    lay->addWidget(gridViewBtn_);

    connect(listViewBtn_, &QToolButton::clicked, this, [this]() { onViewToggled(); });
    connect(gridViewBtn_, &QToolButton::clicked, this, [this]() { onViewToggled(); });
}

void LibraryView::setupList() {
    treeView_ = new QTreeView(this);
    treeView_->setRootIsDecorated(false);
    treeView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView_->setAlternatingRowColors(false);
    treeView_->setSortingEnabled(true);
    treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView_->header()->setStretchLastSection(true);
    treeView_->header()->setSectionsMovable(true);
    treeView_->header()->setDefaultSectionSize(80);
    treeView_->setItemDelegate(new TrackDelegate(treeView_));

    connect(treeView_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        auto src = proxy_->mapToSource(index);
        if (model_) emit trackActivated(model_->trackAt(src.row()));
    });

    connect(treeView_, &QTreeView::clicked, this, [this](const QModelIndex& index) {
        auto src = proxy_->mapToSource(index);
        if (model_) emit trackSelected(model_->trackAt(src.row()));
    });

    connect(treeView_, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu;
        auto tracks = selectedTracks();
        if (tracks.isEmpty()) return;
        if (tracks.size() == 1) {
            menu.addAction("Play Now");
            menu.addAction("Add to Queue");
            auto* playlistMenu = menu.addMenu("Add to Playlist");
            Q_UNUSED(playlistMenu);
            menu.addSeparator();
            menu.addAction("Show in Finder");
            menu.addAction("Edit Tags");
        } else {
            menu.addAction(QString("Play %1 Tracks").arg(tracks.size()));
            menu.addAction(QString("Add %1 Tracks to Queue").arg(tracks.size()));
        }
        menu.exec(treeView_->viewport()->mapToGlobal(pos));
    });
}

void LibraryView::setupGrid() {
    gridView_ = new QListView(this);
    gridView_->setViewMode(QListView::IconMode);
    gridView_->setIconSize(QSize(160, 160));
    gridView_->setGridSize(QSize(180, 200));
    gridView_->setUniformItemSizes(true);
    gridView_->setResizeMode(QListView::Adjust);
    gridView_->setWordWrap(false);
    gridView_->setSpacing(0);
    gridView_->setFlow(QListView::LeftToRight);
    gridView_->setWrapping(true);

    albumDelegate_ = new AlbumDelegate(this);
    gridView_->setItemDelegate(albumDelegate_);
    gridView_->setVisible(false);

    connect(gridView_, &QListView::clicked, this, &LibraryView::onAlbumClicked);
}

void LibraryView::onSearchTextChanged(const QString& text) {
    proxy_->setFilterFixedString(text);
}

void LibraryView::onViewToggled() {
    gridMode_ = !gridMode_;
    gridView_->setVisible(gridMode_);
    listViewBtn_->setChecked(!gridMode_);
    gridViewBtn_->setChecked(gridMode_);
}

void LibraryView::onAlbumClicked(const QModelIndex& index) {
    QString album = index.data(Qt::DisplayRole).toString();
    QString artist = index.data(Qt::UserRole).toString();
    emit albumSelected(album, artist);
}

void LibraryView::setModel(LibraryModel* model) {
    model_ = model;
    proxy_->setSourceModel(model);
    treeView_->setModel(proxy_);
    restoreColumnWidths();
}

QList<TrackMetadata> LibraryView::selectedTracks() const {
    QList<TrackMetadata> result;
    auto sel = treeView_->selectionModel()->selectedRows(0);
    for (const auto& idx : sel) {
        auto src = proxy_->mapToSource(idx);
        if (model_) result.append(model_->trackAt(src.row()));
    }
    return result;
}

void LibraryView::restoreColumnWidths() {
    QSettings settings;
    auto header = treeView_->header();
    for (int i = 0; i < LibraryModel::ColumnCount; ++i) {
        int w = settings.value(QString("Library/ColumnWidth/%1").arg(i), -1).toInt();
        if (w > 0) header->resizeSection(i, w);
    }
    connect(header, &QHeaderView::sectionResized, this, [](int logical, int, int newSize) {
        QSettings().setValue(QString("Library/ColumnWidth/%1").arg(logical), newSize);
    });
}

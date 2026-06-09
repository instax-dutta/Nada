#include "QueueView.h"
#include "Theme.h"
#include "library/CoverArtCache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QAction>

class QueueDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        QRect r = option.rect;
        bool current = index.data(Qt::UserRole + 1).toBool();
        bool hovered = option.state & QStyle::State_MouseOver;

        QColor bg = current ? Theme::CARD : (hovered ? Theme::CARD : Qt::transparent);
        painter->fillRect(r, bg);

        if (current) {
            painter->fillRect(r.x(), r.y(), 2, r.height(), Theme::ACCENT);
        }

        painter->setPen(Theme::BORDER);
        painter->drawLine(r.bottomLeft(), r.bottomRight());

        int pos = index.data(Qt::DisplayRole).toInt();
        QString title = index.data(Qt::UserRole).toString();
        QString artist = index.data(Qt::UserRole + 2).toString();
        QString duration = index.data(Qt::UserRole + 3).toString();

        painter->setPen(Theme::TEXT_SEC);
        painter->setFont(Theme::monoFont());
        QRect numRect(r.x() + 8, r.y(), 24, r.height());
        painter->drawText(numRect, Qt::AlignLeft | Qt::AlignVCenter, QString::number(pos));

        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::CARD);
        painter->drawRoundedRect(r.x() + 32, r.y() + 10, 32, 32, 2, 2);

        painter->setFont(Theme::uiFont());
        painter->setPen(current ? Theme::ACCENT : Theme::TEXT_PRI);
        QRect titleRect(r.x() + 72, r.y(), r.width() - 140, r.height() / 2);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignBottom | Qt::ElideRight, title);

        painter->setPen(Theme::TEXT_SEC);
        QRect artistRect(r.x() + 72, r.y() + r.height() / 2, r.width() - 140, r.height() / 2);
        painter->drawText(artistRect, Qt::AlignLeft | Qt::AlignTop | Qt::ElideRight, artist);

        painter->setPen(Theme::TEXT_SEC);
        painter->setFont(Theme::monoFont());
        QRect durRect(r.x() + r.width() - 80, r.y(), 70, r.height());
        painter->drawText(durRect, Qt::AlignRight | Qt::AlignVCenter, duration);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return QSize(0, 52);
    }
};

QueueView::QueueView(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void QueueView::setupUi() {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto* playingHeader = new QLabel("CURRENTLY PLAYING", this);
    playingHeader->setFont(Theme::monoFont());
    playingHeader->setStyleSheet("color: #86868B; padding: 8px 12px 4px; font-size: 10px;");
    lay->addWidget(playingHeader);

    currentTrackCard_ = new QWidget(this);
    currentTrackCard_->setFixedHeight(60);
    currentTrackCard_->setStyleSheet("background-color: #161616;");
    auto* cardLay = new QHBoxLayout(currentTrackCard_);
    cardLay->setContentsMargins(14, 8, 12, 8);
    cardLay->setSpacing(10);

    auto* accentBar = new QWidget(currentTrackCard_);
    accentBar->setFixedSize(2, 44);
    accentBar->setStyleSheet("background-color: #D4B982;");
    cardLay->addWidget(accentBar);

    currentTrackTitle_ = new QLabel("No track playing", currentTrackCard_);
    currentTrackTitle_->setFont(Theme::uiFont());
    currentTrackTitle_->setStyleSheet("color: #F5F5F7;");
    cardLay->addWidget(currentTrackTitle_, 1);

    currentTrackArtist_ = new QLabel("", currentTrackCard_);
    currentTrackArtist_->setFont(Theme::uiFont());
    currentTrackArtist_->setStyleSheet("color: #86868B;");
    cardLay->addWidget(currentTrackArtist_);

    lay->addWidget(currentTrackCard_);

    auto* upNextHeader = new QLabel("UP NEXT", this);
    upNextHeader->setFont(Theme::monoFont());
    upNextHeader->setStyleSheet("color: #86868B; padding: 8px 12px 4px; font-size: 10px;");
    lay->addWidget(upNextHeader);

    model_ = new QStandardItemModel(this);
    listView_ = new QListView(this);
    listView_->setModel(model_);
    listView_->setItemDelegate(new QueueDelegate(listView_));
    listView_->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_->setDragDropMode(QAbstractItemView::InternalMove);
    listView_->setDefaultDropAction(Qt::MoveAction);
    listView_->setDragDropOverwriteMode(false);
    listView_->setContextMenuPolicy(Qt::CustomContextMenu);
    listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listView_->setStyleSheet(
        "QListView { background: transparent; }"
        "QListView::item { border: none; }");

    connect(listView_, &QListView::clicked, this, [this](const QModelIndex& idx) {
        if (idx.isValid()) emit trackSelected(idx.row());
    });

    connect(listView_, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu;
        menu.addAction("Remove", this, [this]() {
            auto idx = listView_->currentIndex();
            if (idx.isValid()) removeFromQueue(idx.row());
        });
        menu.addAction("Move to Top", []() {});
        menu.addAction("Move to Bottom", []() {});
        menu.exec(listView_->viewport()->mapToGlobal(pos));
    });

    lay->addWidget(listView_, 1);

    auto* footer = new QWidget(this);
    footer->setFixedHeight(32);
    auto* fLay = new QHBoxLayout(footer);
    fLay->setContentsMargins(12, 2, 12, 2);
    footerLabel_ = new QLabel("0 tracks \u00B7 0:00", this);
    footerLabel_->setFont(Theme::uiFont());
    footerLabel_->setStyleSheet("color: #86868B;");
    fLay->addWidget(footerLabel_);
    fLay->addStretch();
    lay->addWidget(footer);
}

void QueueView::setQueue(const QList<TrackMetadata>& tracks) {
    queue_ = tracks;
    updateModel();
    updateFooter();
}

void QueueView::addToQueue(const TrackMetadata& track) {
    queue_.append(track);
    updateModel();
    updateFooter();
}

void QueueView::removeFromQueue(int index) {
    if (index >= 0 && index < queue_.size()) {
        queue_.removeAt(index);
        updateModel();
        updateFooter();
    }
}

void QueueView::clearQueue() {
    queue_.clear();
    currentIndex_ = -1;
    updateModel();
    updateFooter();
}

void QueueView::setCurrentIndex(int index) {
    currentIndex_ = index;
    updateModel();
}

void QueueView::updateModel() {
    model_->clear();
    if (currentIndex_ >= 0 && currentIndex_ < queue_.size()) {
        const auto& t = queue_[currentIndex_];
        currentTrackTitle_->setText(t.displayTitle());
        currentTrackArtist_->setText(t.displayArtist());
        currentTrackCard_->show();
    } else {
        currentTrackTitle_->setText("No track playing");
        currentTrackArtist_->setText("");
    }

    for (int i = 0; i < queue_.size(); ++i) {
        const auto& t = queue_[i];
        auto* item = new QStandardItem();
        item->setData(i + 1, Qt::DisplayRole);
        item->setData(t.displayTitle(), Qt::UserRole);
        item->setData(t.displayArtist(), Qt::UserRole + 2);
        item->setData(t.formattedDuration(), Qt::UserRole + 3);
        item->setData(i == currentIndex_, Qt::UserRole + 1);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        model_->appendRow(item);
    }
}

void QueueView::updateFooter() {
    int64_t totalMs = 0;
    for (const auto& t : queue_) totalMs += t.durationMs;
    int totalSec = static_cast<int>(totalMs / 1000);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    footerLabel_->setText(QString("%1 tracks \u00B7 %2:%3")
                              .arg(queue_.size())
                              .arg(min)
                              .arg(sec, 2, 10, QLatin1Char('0')));
}

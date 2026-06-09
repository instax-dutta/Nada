#pragma once

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QLabel>
#include <QPushButton>
#include "library/TrackMetadata.h"

class QueueView : public QWidget {
    Q_OBJECT
public:
    explicit QueueView(QWidget* parent = nullptr);

    void setQueue(const QList<TrackMetadata>& tracks);
    void addToQueue(const TrackMetadata& track);
    void removeFromQueue(int index);
    void clearQueue();
    void setCurrentIndex(int index);

    QList<TrackMetadata> queue() const { return queue_; }

signals:
    void trackSelected(int index);

private:
    void setupUi();
    void updateModel();
    void updateFooter();

    QListView* listView_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLabel* footerLabel_ = nullptr;
    QPushButton* clearBtn_ = nullptr;

    QWidget* currentTrackCard_ = nullptr;
    QLabel* currentTrackTitle_ = nullptr;
    QLabel* currentTrackArtist_ = nullptr;

    QList<TrackMetadata> queue_;
    int currentIndex_ = -1;
};

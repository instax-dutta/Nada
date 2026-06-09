#pragma once

#include <QString>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

struct ReplayGainResult {
    float gainDb = 0.f;
    float peakLinear = 1.f;
    bool valid = false;
};

class ReplayGainScanner : public QObject {
    Q_OBJECT
public:
    explicit ReplayGainScanner(QObject* parent = nullptr);

    QFuture<ReplayGainResult> scan(const QString& path);

signals:
    void scanFinished(QString path, ReplayGainResult result);

private:
    ReplayGainResult scanInternal(const QString& path);
};

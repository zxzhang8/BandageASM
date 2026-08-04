#ifndef TANGLEPATHWORKER_H
#define TANGLEPATHWORKER_H

#include <QObject>
#include <atomic>
#include "tanglepathsearch.h"

class TanglePathWorker : public QObject
{
    Q_OBJECT

public:
    explicit TanglePathWorker(const TanglePathSearchRequest &request,
                              QObject *parent = 0);

public slots:
    void run();
    void cancel();

signals:
    void finished(const TanglePathSearchResult &result);

private:
    TanglePathSearchRequest m_request;
    std::atomic_bool m_cancelled;
};

#endif // TANGLEPATHWORKER_H

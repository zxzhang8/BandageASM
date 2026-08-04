#include "tanglepathworker.h"

TanglePathWorker::TanglePathWorker(const TanglePathSearchRequest &request,
                                   QObject *parent)
    : QObject(parent), m_request(request), m_cancelled(false)
{
}

void TanglePathWorker::run()
{
    TanglePathSearchResult result;
    if (m_request.algorithm == TANGLE_PATH_CP_SAT)
        result = runCpSatTanglePathSearch(m_request, &m_cancelled);
    else
        result = runBeamTanglePathSearch(m_request, &m_cancelled);
    emit finished(result);
}

void TanglePathWorker::cancel()
{
    m_cancelled.store(true);
}

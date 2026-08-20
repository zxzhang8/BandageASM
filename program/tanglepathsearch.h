#ifndef TANGLEPATHSEARCH_H
#define TANGLEPATHSEARCH_H

#include <QHash>
#include <QList>
#include <QMetaType>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <vector>
#include "../program/gafparser.h"

class DeBruijnNode;

enum TanglePathAlgorithm
{
    TANGLE_PATH_BEAM_SEARCH,
    TANGLE_PATH_CP_SAT
};

struct TangleSegment
{
    QString name;
    int length;
    double coverage;
};

struct TangleEdge
{
    int target;
    int targetOverlap;

    bool operator<(const TangleEdge &other) const
    {
        if (target != other.target)
            return target < other.target;
        return targetOverlap < other.targetOverlap;
    }

    bool operator==(const TangleEdge &other) const
    {
        return target == other.target && targetOverlap == other.targetOverlap;
    }
};

struct TangleGraph
{
    QVector<TangleSegment> segments;
    QHash<QString, int> segmentIndex;
    QVector<int> orientedToSegment;
    QVector<QVector<TangleEdge> > adjacency;

    int orientedId(const QString &name, QChar orientation) const;
    QString label(int orientedId) const;
    bool containsTransition(int left, int right) const;
    int targetOverlap(int left, int right) const;
};

struct TangleReadAlignment
{
    QString readId;
    qint64 queryLength;
    qint64 queryStart;
    qint64 queryEnd;
    int residueMatches;
    int blockLength;
    int mappingQuality;
    bool hasAlignmentScore;
    double alignmentScore;
    double identity;
    QVector<int> path;
};

struct TanglePathParameters
{
    int beamSize;
    int perNodeBeam;
    int maxCopy;
    double tauMin;
    double lambdaMissing;
    double lambdaExtra;
    double lambdaStep;
    double beamHuberDelta;
    int topK;

    double coverageDispersion;
    double cpHuberDelta;
    double cpTauMin;
    double cpSingleCopyCoverage;
    bool cpSingleCopyCoverageLocked;
    double fullThreadFraction;
    double contextFraction;
    int contextMin;
    int contextMax;
    double asFraction;
    double coverageWeight;
    double readWeight;
    double timeLimitSeconds;
    int randomSeed;

    TanglePathParameters();
};

struct TanglePathSearchRequest
{
    TanglePathAlgorithm algorithm;
    TangleGraph graph;
    QString source;
    QString target;
    QVector<TangleReadAlignment> readAlignments;
    TanglePathParameters parameters;
};

struct TanglePathCandidate
{
    QStringList orientedNodeNames;
    double score;
    double coverageMad;
    double explainedLengthFraction;
    double copyAgreement;
    double weightedReadSupport;
};

struct TanglePathSearchResult
{
    QList<TanglePathCandidate> candidates;
    QString status;
    QString errorMessage;
    bool cancelled;
    bool relaxedCoverage;
    qint64 elapsedMs;

    TanglePathSearchResult();
};

QStringList commonNumericGfaTags(const std::vector<DeBruijnNode *> &selectedNodes);

bool buildTangleGraph(const std::vector<DeBruijnNode *> &selectedNodes,
                      const QString &coverageTag,
                      TangleGraph *graph,
                      QStringList *invalidCoverageNodes,
                      QString *errorMessage);

QVector<TangleReadAlignment> extractTangleReadAlignments(
        const QList<GafAlignment> &alignments,
        const TangleGraph &graph,
        int *discardedAlignmentCount = 0);

TanglePathSearchResult runBeamTanglePathSearch(const TanglePathSearchRequest &request,
                                               std::atomic_bool *cancelled = 0);

TanglePathSearchResult runCpSatTanglePathSearch(const TanglePathSearchRequest &request,
                                                std::atomic_bool *cancelled = 0);

Q_DECLARE_METATYPE(TanglePathSearchResult)

#endif // TANGLEPATHSEARCH_H

#include "graphlayoutio.h"

#include <QCryptographicHash>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QByteArrayView>
#endif
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <algorithm>
#include <cmath>
#include "../graph/assemblygraph.h"
#include "../graph/debruijnedge.h"
#include "../graph/debruijnnode.h"

namespace
{
const char *layoutFormat = "BandageASM-layout";
const int layoutVersion = 2;

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != 0)
        *errorMessage = message;
}

void addHashData(QCryptographicHash &hash, const QByteArray &value)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hash.addData(QByteArrayView(value));
#else
    hash.addData(value);
#endif
}

void addFramedValue(QCryptographicHash &hash, const QByteArray &value)
{
    addHashData(hash, QByteArray::number(value.size()));
    addHashData(hash, QByteArrayLiteral(":"));
    addHashData(hash, value);
    addHashData(hash, QByteArrayLiteral("\n"));
}

QByteArray edgeFingerprintRecord(const DeBruijnEdge *edge)
{
    QByteArray record;
    const QList<QByteArray> fields = {
        edge->getStartingNode()->getName().toUtf8(),
        edge->getEndingNode()->getName().toUtf8(),
        QByteArray::number(edge->getOverlap()),
        QByteArray::number(int(edge->getOverlapType()))
    };
    for (const QByteArray &field : fields)
    {
        record += QByteArray::number(field.size()) + ":" + field + "\n";
    }
    return record;
}

bool layoutOrientationIsValid(const SavedGraphLayout &layout,
                              const AssemblyGraph &graph,
                              QString *errorMessage)
{
    QMapIterator<QString, std::vector<QPointF> > node(layout.nodePoints);
    while (node.hasNext())
    {
        node.next();
        DeBruijnNode *graphNode = graph.m_deBruijnGraphNodes.value(node.key(), 0);
        if (graphNode == 0)
        {
            setError(errorMessage, "The loaded graph does not contain node " +
                     node.key() + ".");
            return false;
        }
        if (!layout.doubleMode &&
                layout.nodePoints.contains(graphNode->getReverseComplement()->getName()))
        {
            setError(errorMessage, "Single-node mode cannot contain both orientations of node " +
                     graphNode->getNameWithoutSign() + ".");
            return false;
        }
    }
    return true;
}

bool jsonIntegerEquals(const QJsonValue &value, int expected)
{
    return value.isDouble() && value.toDouble() == double(expected);
}
}

QString graphLayoutFingerprint(const AssemblyGraph &graph)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    addHashData(hash, QByteArrayLiteral("BandageASM graph fingerprint v2\n"));
    QMapIterator<QString, DeBruijnNode *> node(graph.m_deBruijnGraphNodes);
    while (node.hasNext())
    {
        node.next();
        addHashData(hash, QByteArrayLiteral("N\n"));
        addFramedValue(hash, node.key().toUtf8());
        addFramedValue(hash, QByteArray::number(node.value()->getLength()));
        addFramedValue(hash, QCryptographicHash::hash(
                           node.value()->getSequence(), QCryptographicHash::Sha256).toHex());
    }

    QList<QByteArray> edges;
    QMapIterator<QPair<DeBruijnNode *, DeBruijnNode *>, DeBruijnEdge *> edge(
                graph.m_deBruijnGraphEdges);
    while (edge.hasNext())
    {
        edge.next();
        DeBruijnEdge *item = edge.value();
        edges << edgeFingerprintRecord(item);
    }
    std::sort(edges.begin(), edges.end());
    for (const QByteArray &record : edges)
    {
        addHashData(hash, QByteArrayLiteral("E\n"));
        addHashData(hash, record);
    }

    return QString::fromLatin1(hash.result().toHex());
}

bool saveGraphLayout(const QString &fileName,
                     const AssemblyGraph &graph,
                     const SavedGraphLayout &layout,
                     QString *errorMessage)
{
    setError(errorMessage, QString());
    if (layout.nodePoints.isEmpty())
    {
        setError(errorMessage, "The current drawing contains no nodes.");
        return false;
    }
    if (!layoutOrientationIsValid(layout, graph, errorMessage))
        return false;

    QJsonObject nodes;
    QMapIterator<QString, std::vector<QPointF> > node(layout.nodePoints);
    while (node.hasNext())
    {
        node.next();
        QJsonArray points;
        const std::vector<QPointF> &linePoints = node.value();
        if (linePoints.size() < 2)
        {
            setError(errorMessage, "Node " + node.key() +
                     " has fewer than two layout points.");
            return false;
        }
        for (size_t i = 0; i < linePoints.size(); ++i)
        {
            if (!std::isfinite(linePoints[i].x()) || !std::isfinite(linePoints[i].y()))
            {
                setError(errorMessage, "Node " + node.key() +
                         " contains a non-finite coordinate.");
                return false;
            }
            QJsonArray point;
            point.append(linePoints[i].x());
            point.append(linePoints[i].y());
            points.append(point);
        }
        nodes.insert(node.key(), points);
    }

    QJsonObject graphInfo;
    graphInfo.insert("fingerprint", graphLayoutFingerprint(graph));
    graphInfo.insert("nodeCount", graph.m_deBruijnGraphNodes.size());
    graphInfo.insert("edgeCount", graph.m_deBruijnGraphEdges.size());

    QJsonObject display;
    display.insert("doubleMode", layout.doubleMode);

    QJsonObject root;
    root.insert("format", layoutFormat);
    root.insert("version", layoutVersion);
    root.insert("graph", graphInfo);
    root.insert("display", display);
    root.insert("nodes", nodes);

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        setError(errorMessage, "Could not open the file for writing: " + file.errorString());
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Compact)) < 0)
    {
        setError(errorMessage, "Could not write the layout: " + file.errorString());
        file.cancelWriting();
        return false;
    }
    if (!file.commit())
    {
        setError(errorMessage, "Could not finish writing the layout: " + file.errorString());
        return false;
    }
    return true;
}

bool loadGraphLayout(const QString &fileName,
                     const AssemblyGraph &graph,
                     SavedGraphLayout *layout,
                     QString *errorMessage)
{
    setError(errorMessage, QString());
    if (layout == 0)
    {
        setError(errorMessage, "No destination was provided for the layout.");
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        setError(errorMessage, "Could not open the file: " + file.errorString());
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        setError(errorMessage, "Invalid JSON at offset " +
                 QString::number(parseError.offset) + ": " + parseError.errorString());
        return false;
    }
    if (!document.isObject())
    {
        setError(errorMessage, "The layout root must be a JSON object.");
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value("format").toString() != layoutFormat)
    {
        setError(errorMessage, "This is not a BandageASM layout file.");
        return false;
    }
    if (!jsonIntegerEquals(root.value("version"), layoutVersion))
    {
        setError(errorMessage, "Unsupported BandageASM layout version. "
                 "This build accepts version 2 layouts only.");
        return false;
    }
    if (!root.value("graph").isObject() || !root.value("display").isObject() ||
            !root.value("nodes").isObject())
    {
        setError(errorMessage, "The layout is missing graph, display or node data.");
        return false;
    }

    const QJsonObject graphInfo = root.value("graph").toObject();
    const QString fingerprint = graphInfo.value("fingerprint").toString();
    if (!jsonIntegerEquals(graphInfo.value("nodeCount"),
                           graph.m_deBruijnGraphNodes.size()) ||
            !jsonIntegerEquals(graphInfo.value("edgeCount"),
                               graph.m_deBruijnGraphEdges.size()))
    {
        setError(errorMessage, "The layout graph dimensions do not match the loaded graph.");
        return false;
    }
    if (fingerprint.size() != 64 ||
            !QRegularExpression("^[0-9a-f]{64}$").match(fingerprint).hasMatch() ||
            fingerprint != graphLayoutFingerprint(graph))
    {
        setError(errorMessage, "The layout belongs to a different graph. "
                 "Its graph fingerprint does not match the loaded graph.");
        return false;
    }

    const QJsonObject display = root.value("display").toObject();
    if (!display.value("doubleMode").isBool())
    {
        setError(errorMessage, "The layout contains an invalid display mode.");
        return false;
    }

    SavedGraphLayout parsed;
    parsed.doubleMode = display.value("doubleMode").toBool();
    const QJsonObject nodes = root.value("nodes").toObject();
    if (nodes.isEmpty())
    {
        setError(errorMessage, "The layout contains no visible nodes.");
        return false;
    }
    for (QJsonObject::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
    {
        if (!graph.m_deBruijnGraphNodes.contains(node.key()))
        {
            setError(errorMessage, "The loaded graph does not contain node " + node.key() + ".");
            return false;
        }
        if (!node.value().isArray())
        {
            setError(errorMessage, "The layout points for node " + node.key() +
                     " must be an array.");
            return false;
        }
        const QJsonArray points = node.value().toArray();
        if (points.size() < 2)
        {
            setError(errorMessage, "Node " + node.key() +
                     " has fewer than two layout points.");
            return false;
        }
        std::vector<QPointF> linePoints;
        linePoints.reserve(points.size());
        for (int i = 0; i < points.size(); ++i)
        {
            if (!points[i].isArray())
            {
                setError(errorMessage, "A coordinate for node " + node.key() +
                         " is not an array.");
                return false;
            }
            const QJsonArray point = points[i].toArray();
            if (point.size() != 2 || !point[0].isDouble() || !point[1].isDouble())
            {
                setError(errorMessage, "A coordinate for node " + node.key() +
                         " must contain exactly two numbers.");
                return false;
            }
            const double x = point[0].toDouble();
            const double y = point[1].toDouble();
            if (!std::isfinite(x) || !std::isfinite(y))
            {
                setError(errorMessage, "Node " + node.key() +
                         " contains a non-finite coordinate.");
                return false;
            }
            linePoints.push_back(QPointF(x, y));
        }
        parsed.nodePoints.insert(node.key(), linePoints);
    }

    if (!layoutOrientationIsValid(parsed, graph, errorMessage))
        return false;

    *layout = parsed;
    return true;
}

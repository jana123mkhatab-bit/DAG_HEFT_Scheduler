#pragma once
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QSet>
#include "DataTypes.h"

class DagScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit DagScene(QObject* parent = nullptr);

    void loadDAG(const DAGData& dag);
    void applyResult(const AlgorithmResult& result);   // colour nodes by VM
    void resetColors();
    void highlightTask(int taskId);
    void clearHighlight();
    void highlightCriticalPath(const QVector<int>& path);
    void showStepTask(int taskId);   // dim all, highlight this one

signals:
    void taskSelected(int taskId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

private:
    struct NodeInfo {
        QGraphicsEllipseItem* ellipse = nullptr;
        QGraphicsTextItem*    label   = nullptr;
        QGraphicsEllipseItem* glow    = nullptr;
        QPointF               center;
    };

    void buildLayout();
    void drawEdge(QPointF from, QPointF to, bool isCritical = false);
    QVector<QVector<int>> computeLevels() const;
    void setNodeColor(int taskId, QColor fill, QColor rim, double glowAlpha=0.15);

    QMap<int, NodeInfo> m_nodes;
    DAGData             m_dag;
    int                 m_highlighted = -1;
    QSet<int>           m_criticalPath;
    QMap<int,int>       m_taskVm;      // taskId → vmId (after scheduling)

    // Edge items (for critical path re-colouring)
    struct EdgeItem { int from,to; QGraphicsItem* line; QGraphicsItem* arrow; };
    QVector<EdgeItem> m_edges;

    static constexpr double R       = 26.0;
    static constexpr double H_GAP   = 140.0;
    static constexpr double V_GAP   = 80.0;
};

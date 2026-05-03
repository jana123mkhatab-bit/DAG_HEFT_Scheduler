#pragma once
#include <QGraphicsScene>
#include "DataTypes.h"

class GanttScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GanttScene(QObject* parent = nullptr);
    void loadResult(const AlgorithmResult& result, const DAGData& dag);
    void showUpToStep(int step);   // show only first N entries
    void clear();

signals:
    void taskClicked(int taskId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

private:
    void drawBackground(double maxTime, int numVMs,
                        double rowH, double xs, double xOff, double yOff);
    void drawMakespanLine(double ms, double xs, double xOff, double yOff, double totalH);
    void drawDependencyArrows(double xs, double xOff, double yOff, double rowH);

    AlgorithmResult m_result;
    DAGData         m_dag;
    // Keep block items for step-by-step
    QVector<QGraphicsItem*> m_blocks;

    static constexpr double ROW_H  = 56.0;
    static constexpr double X_OFF  = 85.0;
    static constexpr double Y_OFF  = 45.0;
    static constexpr double MAX_W  = 750.0;
};

#pragma once
#include <QWidget>
#include <QLabel>
#include "DataTypes.h"

// Shows task details when a node is clicked
class TaskDetailsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TaskDetailsPanel(QWidget* parent = nullptr);
    void showTask(int taskId, const DAGData& dag, const AlgorithmResult& result);
    void clear();

private:
    QLabel* m_title;
    QLabel* m_preds;
    QLabel* m_succs;
    QLabel* m_execTimes;
    QLabel* m_assignedVM;
    QLabel* m_timing;
};

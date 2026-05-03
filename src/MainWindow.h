#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QGraphicsView>
#include <QTabWidget>
#include "ControlPanel.h"
#include "ComparisonPanel.h"
#include "TaskDetailsPanel.h"
#include "DagScene.h"
#include "GanttScene.h"
#include "AlgorithmEngine.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onGenerate(int numTasks, int numVMs, unsigned seed);
    void onRunHEFT();
    void onRunDP();
    void onRunDC();
    void onExportGantt();
    void onExportCSV();
    void onStepForward();
    void onStepBackward();
    void onStepReset();
    void onAlgoToggle(int idx);
    void onTaskSelected(int taskId);
    void onGanttTaskClicked(int taskId);

private:
    void setupUI();
    void runAlgorithm(int idx);
    void showResult(int idx);
    void updateStepLabel();

    // UI
    ControlPanel*    m_ctrl;
    ComparisonPanel* m_comp;
    TaskDetailsPanel*m_taskDetails;
    QGraphicsView*   m_dagView;
    DagScene*        m_dagScene;
    QTabWidget*      m_ganttTabs;
    GanttScene*      m_ganttScenes[3];
    QGraphicsView*   m_ganttViews[3];

    // Data
    DAGData          m_dag;
    AlgorithmResult  m_results[3];
    int              m_activeAlgo  = 0;
    int              m_stepCurrent = 0;
    int              m_stepTotal   = 0;
    int              m_selectedTask= -1;
};

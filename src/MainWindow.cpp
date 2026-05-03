#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPixmap>
#include <QPainter>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Cloud Task Scheduling Visualizer — HEFT, DP & D&C");
    resize(1440, 860);
    setStyleSheet("QMainWindow { background:#0E1120; }");
    setupUI();
    onGenerate(10, 4, 42);
}

// ── UI Setup ──────────────────────────────────────────────────────────────────
void MainWindow::setupUI()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outerL = new QVBoxLayout(central);
    outerL->setContentsMargins(0,0,0,0);
    outerL->setSpacing(0);

    // ── Main horizontal splitter ────────────────────────────────────────────
    auto* hSplit = new QSplitter(Qt::Horizontal, this);
    hSplit->setHandleWidth(2);
    hSplit->setStyleSheet("QSplitter::handle { background:#1E2436; }");

    // ── LEFT: ControlPanel + TaskDetailsPanel stacked (width matched exactly) ──
    auto* leftWidget = new QWidget(this);
    leftWidget->setFixedWidth(280);   // must match ControlPanel::setFixedWidth
    leftWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* leftL = new QVBoxLayout(leftWidget);
    leftL->setContentsMargins(0,0,0,0);
    leftL->setSpacing(0);
    m_ctrl = new ControlPanel(leftWidget);
    m_taskDetails = new TaskDetailsPanel(leftWidget);
    m_taskDetails->setFixedHeight(230);
    leftL->addWidget(m_ctrl, 1);      // ctrl takes all remaining height
    leftL->addWidget(m_taskDetails);  // task panel pinned at bottom
    hSplit->addWidget(leftWidget);

    // ── CENTER: DAG view ────────────────────────────────────────────────────
    m_dagScene = new DagScene(this);
    m_dagView  = new QGraphicsView(m_dagScene, this);
    m_dagView->setRenderHint(QPainter::Antialiasing);
    m_dagView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_dagView->setStyleSheet("QGraphicsView { border:none; background:#12161E; }");
    m_dagView->setMinimumWidth(280);
    hSplit->addWidget(m_dagView);

    // ── RIGHT: Gantt tabs on top, comparison panel pinned below ────────────
    //   Wrap in a QWidget with a vertical layout so the comparison panel
    //   is always inside the same column — never floats behind anything.
    auto* rightWidget = new QWidget(this);
    rightWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* rightL = new QVBoxLayout(rightWidget);
    rightL->setContentsMargins(0,0,0,0);
    rightL->setSpacing(0);

    m_ganttTabs = new QTabWidget(rightWidget);
    m_ganttTabs->setStyleSheet(
        "QTabWidget::pane { border:none; background:#12161E; }"
        "QTabBar::tab { background:#1A1E2E; color:#99AABB; padding:6px 14px; border:none; }"
        "QTabBar::tab:selected { background:#2A3048; color:#CCDDFF; font-weight:bold; }"
        "QTabBar::tab:hover { background:#222840; }");

    static const char* tabNames[] = {"HEFT", "HEFT + DP", "HEFT + D&C"};
    for (int i = 0; i < 3; i++) {
        m_ganttScenes[i] = new GanttScene(this);
        m_ganttViews[i]  = new QGraphicsView(m_ganttScenes[i], rightWidget);
        m_ganttViews[i]->setRenderHint(QPainter::Antialiasing);
        m_ganttViews[i]->setDragMode(QGraphicsView::ScrollHandDrag);
        m_ganttViews[i]->setStyleSheet("QGraphicsView { border:none; background:#12161E; }");
        m_ganttTabs->addTab(m_ganttViews[i], tabNames[i]);
        connect(m_ganttScenes[i], &GanttScene::taskClicked, this, &MainWindow::onGanttTaskClicked);
    }
    rightL->addWidget(m_ganttTabs, 1);   // Gantt fills all space above

    // Comparison panel sits at the very bottom of the right column
    m_comp = new ComparisonPanel(rightWidget);
    rightL->addWidget(m_comp, 0);        // fixed height, never squeezed

    hSplit->addWidget(rightWidget);
    hSplit->setSizes({280, 430, 730});

    outerL->addWidget(hSplit, 1);

    // ── Signal connections ──────────────────────────────────────────────────
    connect(m_ctrl, &ControlPanel::generateRequested,     this, &MainWindow::onGenerate);
    connect(m_ctrl, &ControlPanel::runHEFTRequested,      this, &MainWindow::onRunHEFT);
    connect(m_ctrl, &ControlPanel::runDPRequested,        this, &MainWindow::onRunDP);
    connect(m_ctrl, &ControlPanel::runDCRequested,        this, &MainWindow::onRunDC);
    connect(m_ctrl, &ControlPanel::exportGanttRequested,  this, &MainWindow::onExportGantt);
    connect(m_ctrl, &ControlPanel::exportCSVRequested,    this, &MainWindow::onExportCSV);
    connect(m_ctrl, &ControlPanel::stepForwardRequested,  this, &MainWindow::onStepForward);
    connect(m_ctrl, &ControlPanel::stepBackwardRequested, this, &MainWindow::onStepBackward);
    connect(m_ctrl, &ControlPanel::stepResetRequested,    this, &MainWindow::onStepReset);
    connect(m_ctrl, &ControlPanel::activeAlgorithmChanged,this, &MainWindow::onAlgoToggle);
    connect(m_dagScene, &DagScene::taskSelected,          this, &MainWindow::onTaskSelected);
    connect(m_ganttTabs,&QTabWidget::currentChanged,      this, &MainWindow::onAlgoToggle);
}

// ── Actions ───────────────────────────────────────────────────────────────────
void MainWindow::onGenerate(int numTasks, int numVMs, unsigned seed)
{
    m_ctrl->appendLog(QString("> Generating DAG: %1 tasks, %2 VMs, seed=%3")
                      .arg(numTasks).arg(numVMs).arg(seed), "#66AAFF");
    m_dag = AlgorithmEngine::generateDAG(numTasks, numVMs, 0.5, seed);
    m_dagScene->loadDAG(m_dag);
    for (int i=0;i<3;i++) { m_ganttScenes[i]->clear(); m_results[i]=AlgorithmResult{}; }
    m_comp->clear();
    m_taskDetails->clear();
    m_stepCurrent = m_stepTotal = 0;
    m_selectedTask = -1;
    m_ctrl->appendLog(QString("  Label: %1").arg(m_dag.label), "#88CC88");
}

void MainWindow::onRunHEFT() { runAlgorithm(0); }
void MainWindow::onRunDP()   { runAlgorithm(1); }
void MainWindow::onRunDC()   { runAlgorithm(2); }

void MainWindow::runAlgorithm(int idx)
{
    if (m_dag.tasks.isEmpty()) {
        m_ctrl->appendLog("Error: Generate a DAG first!", "#FF5555");
        return;
    }
    static const char* names[] = {"HEFT","HEFT + Dynamic Programming","HEFT + D&C"};
    m_ctrl->appendLog(QString("> Running %1...").arg(names[idx]), "#AACCFF");

    switch(idx) {
        case 0: m_results[0] = AlgorithmEngine::runHEFT(m_dag);    break;
        case 1: m_results[1] = AlgorithmEngine::runHEFT_DP(m_dag); break;
        case 2: m_results[2] = AlgorithmEngine::runHEFT_DC(m_dag); break;
    }
    const auto& r = m_results[idx];
    m_ctrl->appendLog(QString("  Makespan: %1  |  Runtime: %2 ms  |  Critical path: %3 tasks")
                      .arg(r.makespan,0,'f',2).arg(r.runtimeMs,0,'f',3).arg(r.criticalPath.size()),
                      "#88FF88");

    // Update Gantt for this algo
    m_ganttScenes[idx]->loadResult(r, m_dag);
    m_comp->setResult(idx, r);

    // Switch to this algorithm's tab
    m_ganttTabs->setCurrentIndex(idx);
    m_activeAlgo = idx;

    // Colour DAG nodes by VM assignment
    m_dagScene->applyResult(r);

    // Update task details if a task is selected
    if (m_selectedTask >= 0)
        m_taskDetails->showTask(m_selectedTask, m_dag, r);

    // Prepare step mode
    m_stepTotal   = r.entries.size();
    m_stepCurrent = m_stepTotal; // show all by default
    m_ctrl->setStepBounds(m_stepTotal);
    updateStepLabel();
}

void MainWindow::showResult(int idx)
{
    m_activeAlgo = idx;
    m_ganttTabs->setCurrentIndex(idx);
    if (m_results[idx].isValid) {
        m_dagScene->applyResult(m_results[idx]);
        m_stepTotal   = m_results[idx].entries.size();
        m_stepCurrent = m_stepTotal;
        updateStepLabel();
        if (m_selectedTask >= 0)
            m_taskDetails->showTask(m_selectedTask, m_dag, m_results[idx]);
    }
}

void MainWindow::onAlgoToggle(int idx)
{
    m_ganttTabs->setCurrentIndex(idx);
    showResult(idx);
}

void MainWindow::onStepForward()
{
    if (!m_results[m_activeAlgo].isValid) return;
    m_stepCurrent = std::min(m_stepCurrent+1, m_stepTotal);
    m_ganttScenes[m_activeAlgo]->showUpToStep(m_stepCurrent);
    // Highlight the just-added task in DAG
    if (m_stepCurrent > 0) {
        // entries are sorted by startTime in GanttScene; we need to find the m_stepCurrent-th
        auto sorted = m_results[m_activeAlgo].entries;
        std::sort(sorted.begin(),sorted.end(),[](const ScheduleEntry& a,const ScheduleEntry& b){
            return a.startTime<b.startTime;
        });
        if (m_stepCurrent-1 < sorted.size()) {
            int tid = sorted[m_stepCurrent-1].taskId;
            m_dagScene->showStepTask(tid);
        }
    }
    updateStepLabel();
}

void MainWindow::onStepBackward()
{
    if (!m_results[m_activeAlgo].isValid) return;
    m_stepCurrent = std::max(0, m_stepCurrent-1);
    m_ganttScenes[m_activeAlgo]->showUpToStep(m_stepCurrent);
    updateStepLabel();
}

void MainWindow::onStepReset()
{
    m_stepCurrent = 0;
    if (m_results[m_activeAlgo].isValid)
        m_ganttScenes[m_activeAlgo]->showUpToStep(0);
    m_dagScene->resetColors();
    updateStepLabel();
}

void MainWindow::updateStepLabel()
{
    m_ctrl->updateStepLabel(m_stepCurrent, m_stepTotal);
}


void MainWindow::onTaskSelected(int taskId)
{
    m_selectedTask = taskId;
    m_taskDetails->showTask(taskId, m_dag, m_results[m_activeAlgo]);
    m_ctrl->appendLog(QString("Selected: T%1").arg(taskId), "#FFCC66");
}

void MainWindow::onGanttTaskClicked(int taskId)
{
    m_dagScene->highlightTask(taskId);
    onTaskSelected(taskId);
}

void MainWindow::onExportGantt()
{
    int idx = m_activeAlgo;
    if (!m_results[idx].isValid) {
        QMessageBox::warning(this,"Export","Run an algorithm first.");
        return;
    }
    QString fn = QFileDialog::getSaveFileName(this,"Export Gantt","GanttChart.png","Images (*.png)");
    if (fn.isEmpty()) return;
    QPixmap px(m_ganttScenes[idx]->sceneRect().size().toSize());
    px.fill(QColor(0x12,0x16,0x24));
    QPainter p(&px); p.setRenderHint(QPainter::Antialiasing);
    m_ganttScenes[idx]->render(&p);
    if (px.save(fn)) m_ctrl->appendLog("> Gantt exported: "+fn,"#88FF88");
    else             m_ctrl->appendLog("> Export failed!","#FF5555");
}

void MainWindow::onExportCSV()
{
    QString fn = QFileDialog::getSaveFileName(this,"Export CSV","results.csv","CSV (*.csv)");
    if (fn.isEmpty()) return;
    QFile f(fn); if (!f.open(QIODevice::WriteOnly|QIODevice::Text)) return;
    QTextStream ts(&f);
    ts << "Algorithm,TaskID,VMID,StartTime,FinishTime,Duration\n";
    for (int i=0;i<3;i++) {
        if (!m_results[i].isValid) continue;
        for (const auto& se : m_results[i].entries)
            ts << m_results[i].algorithmName << ","
               << se.taskId << "," << se.vmId << ","
               << se.startTime << "," << se.finishTime << ","
               << (se.finishTime-se.startTime) << "\n";
    }
    m_ctrl->appendLog("> CSV exported: "+fn,"#88FF88");
}

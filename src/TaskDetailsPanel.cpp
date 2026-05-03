#include "TaskDetailsPanel.h"
#include <QVBoxLayout>
#include <QFrame>

static QLabel* makeRow(QWidget* parent, QVBoxLayout* layout, const QString& title)
{
    auto* titleLbl = new QLabel(title, parent);
    titleLbl->setStyleSheet("color:#7799BB; font-size:10px; font-weight:bold; border:none;");
    layout->addWidget(titleLbl);
    auto* val = new QLabel("—", parent);
    val->setStyleSheet("color:#DDEEFF; font-size:11px; border:none; padding-left:6px;");
    val->setWordWrap(true);
    layout->addWidget(val);
    return val;
}

TaskDetailsPanel::TaskDetailsPanel(QWidget* parent) : QWidget(parent)
{
    setStyleSheet("background-color:#181C2A; border-top:1px solid #2A3048;");
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10,10,10,10);
    layout->setSpacing(4);

    auto* hdr = new QLabel("Task Details", this);
    hdr->setStyleSheet("color:#CCDDFF; font-weight:bold; font-size:13px; border:none;");
    layout->addWidget(hdr);
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#2A3048;");
    layout->addWidget(sep);

    m_title     = makeRow(this, layout, "Task");
    m_preds     = makeRow(this, layout, "Parents");
    m_succs     = makeRow(this, layout, "Children");
    m_execTimes = makeRow(this, layout, "Exec Times (per VM)");
    m_assignedVM= makeRow(this, layout, "Assigned VM");
    m_timing    = makeRow(this, layout, "Start → Finish");
    layout->addStretch();
}

void TaskDetailsPanel::showTask(int taskId, const DAGData& dag, const AlgorithmResult& result)
{
    if (taskId < 0 || taskId >= dag.tasks.size()) { clear(); return; }
    const auto& t = dag.tasks[taskId];
    m_title->setText(QString("<b>%1</b>").arg(t.name));

    auto fmt = [](const QVector<int>& ids) {
        if (ids.isEmpty()) return QString("none");
        QString s; for (int id : ids) s += QString("T%1  ").arg(id);
        return s.trimmed();
    };
    m_preds->setText(fmt(t.predecessors));
    m_succs->setText(fmt(t.successors));

    QString et;
    for (int v=0; v<t.execTimes.size(); v++)
        et += QString("<span style='color:%1'>VM%2:%.2f</span>  ")
              .arg(getVMColor(v).name())
              .arg(v+1)
              .arg(t.execTimes[v]);
    m_execTimes->setText(et);

    // Find scheduling info
    const ScheduleEntry* se = nullptr;
    for (const auto& entry : result.entries)
        if (entry.taskId == taskId) { se = &entry; break; }

    if (se && result.isValid) {
        QColor vc = getVMColor(se->vmId);
        m_assignedVM->setText(QString("<span style='color:%1'>VM %2</span>")
                              .arg(vc.name()).arg(se->vmId+1));
        m_timing->setText(QString("%1 → %2  (dur: %3)")
                          .arg(se->startTime,0,'f',2)
                          .arg(se->finishTime,0,'f',2)
                          .arg(se->finishTime-se->startTime,0,'f',2));
    } else {
        m_assignedVM->setText("Not scheduled yet");
        m_timing->setText("—");
    }
}

void TaskDetailsPanel::clear()
{
    m_title->setText("—"); m_preds->setText("—"); m_succs->setText("—");
    m_execTimes->setText("—"); m_assignedVM->setText("—"); m_timing->setText("—");
}

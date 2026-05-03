#include "GanttScene.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <algorithm>
#include <cmath>

GanttScene::GanttScene(QObject* parent) : QGraphicsScene(parent)
{
    setBackgroundBrush(QColor(0x12,0x16,0x24));
}

void GanttScene::clear()
{
    QGraphicsScene::clear();
    m_blocks.clear();
    m_result = AlgorithmResult{};
    m_dag = DAGData{};
}

void GanttScene::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    for (auto* item : items(e->scenePos()))
        if (item->data(0).isValid()) { emit taskClicked(item->data(0).toInt()); return; }
    QGraphicsScene::mousePressEvent(e);
}

// ── Grid background ──────────────────────────────────────────────────────────
void GanttScene::drawBackground(double maxTime, int numVMs,
                                 double rowH, double xs,
                                 double xOff, double yOff)
{
    QFont lf("Segoe UI", 8);
    QColor gridC(0x28,0x30,0x48);
    QColor axisC(0x44,0x55,0x77);

    double totalW = maxTime * xs + 30;

    for (int v=0; v<numVMs; v++) {
        double y = yOff + v*rowH;
        QColor bg = v%2==0 ? QColor(0x1A,0x1E,0x2E) : QColor(0x1E,0x23,0x36);
        addRect(xOff,y,totalW,rowH,QPen(Qt::NoPen),QBrush(bg))->setZValue(-3);
        addLine(xOff,y+rowH,xOff+totalW,y+rowH,QPen(gridC,1))->setZValue(-2);

        // VM label with colour swatch
        QColor vmC = getVMColor(v);
        addRect(4,y+(rowH-14)/2,8,14,QPen(Qt::NoPen),QBrush(vmC))->setZValue(2);
        auto* lbl = addText(QString("VM %1").arg(v+1), lf);
        lbl->setDefaultTextColor(QColor(0x99,0xBB,0xDD));
        lbl->setPos(16, y+(rowH-lbl->boundingRect().height())/2);
        lbl->setZValue(2);
    }

    // Time ticks
    int step = std::max(1,(int)(maxTime/14));
    for (int t=0; t<=(int)maxTime+step; t+=step) {
        double x = xOff+t*xs;
        addLine(x,yOff,x,yOff+numVMs*rowH,QPen(gridC,1,Qt::DotLine))->setZValue(-2);
        auto* tick = addText(QString::number(t), lf);
        tick->setDefaultTextColor(QColor(0x55,0x66,0x88));
        tick->setPos(x-tick->boundingRect().width()/2, yOff+numVMs*rowH+4);
    }

    // Axes
    double totalH = numVMs*rowH;
    addLine(xOff,yOff,xOff,yOff+totalH,QPen(axisC,2));
    addLine(xOff,yOff+totalH,xOff+totalW,yOff+totalH,QPen(axisC,2));

    // "Time" label
    QFont af("Segoe UI",9,QFont::Bold);
    auto* tl = addText("Time →",af);
    tl->setDefaultTextColor(QColor(0x77,0x99,0xBB));
    tl->setPos(xOff+totalW/2,yOff+totalH+22);
}

void GanttScene::drawMakespanLine(double ms, double xs,
                                   double xOff, double yOff, double totalH)
{
    double x = xOff+ms*xs;
    addLine(x,yOff-12,x,yOff+totalH,QPen(QColor(0xFF,0x44,0x44),2,Qt::DashLine));
    QFont f("Segoe UI",8,QFont::Bold);
    auto* lbl = addText(QString("Makespan\n%1").arg(ms,0,'f',1),f);
    lbl->setDefaultTextColor(QColor(0xFF,0x88,0x88));
    lbl->setPos(x+3,yOff-36);
    lbl->setZValue(5);
}

void GanttScene::drawDependencyArrows(double xs, double xOff,
                                       double yOff, double rowH)
{
    // For each edge (pred → succ), draw a small arrow from end of pred block to start of succ block
    QMap<int,ScheduleEntry> sched;
    for (const auto& se : m_result.entries) sched[se.taskId]=se;
    QSet<int> cpSet(m_result.criticalPath.begin(), m_result.criticalPath.end());

    for (const auto& task : m_dag.tasks) {
        if (!sched.contains(task.id)) continue;
        const auto& se = sched[task.id];
        for (int s : task.successors) {
            if (!sched.contains(s)) continue;
            const auto& sse = sched[s];
            bool isCrit = cpSet.contains(task.id) && cpSet.contains(s);
            QColor c = isCrit ? QColor(0xFF,0x55,0x55,180) : QColor(0x66,0x88,0xAA,120);

            double x1 = xOff+se.finishTime*xs;
            double y1 = yOff+se.vmId*rowH+rowH/2;
            double x2 = xOff+sse.startTime*xs;
            double y2 = yOff+sse.vmId*rowH+rowH/2;

            // Only draw if there's visible space
            if (std::abs(x2-x1) < 2 && std::abs(y2-y1) < 2) continue;
            addLine(x1,y1,x2,y2,QPen(c,1.2,Qt::DotLine))->setZValue(3);
        }
    }
}

// ── Main load ─────────────────────────────────────────────────────────────────
void GanttScene::loadResult(const AlgorithmResult& result, const DAGData& dag)
{
    QGraphicsScene::clear();
    m_blocks.clear();
    if (!result.isValid || result.entries.isEmpty()) return;
    m_result = result;
    m_dag    = dag;

    int    m       = result.numVMs;
    double maxTime = result.makespan;
    double xs      = std::min(MAX_W / std::max(maxTime,1.0), 22.0);
    double xOff    = X_OFF, yOff = Y_OFF;
    double rowH    = ROW_H, totalH = m*rowH;

    drawBackground(maxTime, m, rowH, xs, xOff, yOff);

    QFont tf("Segoe UI",8,QFont::Bold), sf("Segoe UI",7);
    QSet<int> cpSet(result.criticalPath.begin(),result.criticalPath.end());

    // Sort entries by startTime for step-by-step
    QVector<ScheduleEntry> sorted = result.entries;
    std::sort(sorted.begin(),sorted.end(),[](const ScheduleEntry& a,const ScheduleEntry& b){
        return a.startTime < b.startTime;
    });

    for (const auto& se : sorted) {
        double x = xOff+se.startTime*xs, w = (se.finishTime-se.startTime)*xs;
        double y = yOff+se.vmId*rowH+3, h = rowH-6;
        QColor base = getVMColor(se.vmId);

        // Critical path: extra outline
        QPen outPen = cpSet.contains(se.taskId) ? QPen(QColor(0xFF,0x44,0x44),2.5)
                                                 : QPen(base.lighter(150),1.2);
        auto* rect = addRect(x,y,w,h,outPen,QBrush(base));
        rect->setZValue(1);
        rect->setData(0,se.taskId);

        // Shine
        if (w>4) addRect(x+1,y+1,w-2,h*0.3,QPen(Qt::NoPen),
                         QBrush(QColor(255,255,255,25)))->setZValue(2);

        // Labels
        QString main = QString("T%1").arg(se.taskId);
        auto* tl = addText(main,tf);
        tl->setDefaultTextColor(Qt::white);
        QRectF tbr=tl->boundingRect();
        if (tbr.width()<w-4){
            tl->setPos(x+(w-tbr.width())/2, y+(h/2-tbr.height())/2);
            tl->setZValue(3);
            QString sub=QString("[%1→%2]").arg(se.startTime,0,'f',1).arg(se.finishTime,0,'f',1);
            auto* sl=addText(sub,sf);
            sl->setDefaultTextColor(QColor(255,255,255,180));
            QRectF sbr=sl->boundingRect();
            if(sbr.width()<w-4){ sl->setPos(x+(w-sbr.width())/2,y+h/2+1); sl->setZValue(3); }
            else delete sl;
        } else delete tl;

        m_blocks.push_back(rect);
    }

    drawDependencyArrows(xs,xOff,yOff,rowH);
    drawMakespanLine(maxTime,xs,xOff,yOff,totalH);

    // Header
    QFont hf("Segoe UI",11,QFont::Bold);
    auto* hdr=addText(result.algorithmName,hf);
    hdr->setDefaultTextColor(QColor(0xCC,0xDD,0xFF));
    hdr->setPos(xOff,6);
    hdr->setZValue(4);

    // Runtime sub-header
    QFont rf("Segoe UI",8);
    auto* rt=addText(QString("Runtime: %1 ms  |  Makespan: %2")
                     .arg(result.runtimeMs,0,'f',3)
                     .arg(result.makespan,0,'f',2),rf);
    rt->setDefaultTextColor(QColor(0x88,0xAA,0xCC));
    rt->setPos(xOff,22);
    rt->setZValue(4);

    setSceneRect(itemsBoundingRect().adjusted(-20,-20,40,40));
}

void GanttScene::showUpToStep(int step)
{
    for (int i=0; i<m_blocks.size(); i++)
        m_blocks[i]->setVisible(i < step);
}

#include "DagScene.h"
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QGraphicsPolygonItem>
#include <QGraphicsSceneMouseEvent>
#include <QtMath>
#include <algorithm>

DagScene::DagScene(QObject* parent) : QGraphicsScene(parent)
{
    setBackgroundBrush(QColor(0x12, 0x16, 0x24));
}

// ── Public API ────────────────────────────────────────────────────────────────
void DagScene::loadDAG(const DAGData& dag)
{
    clear(); m_nodes.clear(); m_edges.clear();
    m_taskVm.clear(); m_criticalPath.clear(); m_highlighted = -1;
    m_dag = dag;
    buildLayout();
}

void DagScene::applyResult(const AlgorithmResult& result)
{
    m_taskVm.clear();
    for (const auto& se : result.entries) m_taskVm[se.taskId] = se.vmId;

    for (const auto& se : result.entries) {
        QColor c = getVMColor(se.vmId);
        setNodeColor(se.taskId, c, c.lighter(150), 0.25);
    }
    highlightCriticalPath(result.criticalPath);
}

void DagScene::resetColors()
{
    m_taskVm.clear(); m_criticalPath.clear();
    for (int id : m_nodes.keys()) {
        QColor c = getDefaultTaskColor(id);
        setNodeColor(id, c, c.lighter(140), 0.15);
    }
    // Reset edge colours
    for (auto& e : m_edges) {
        if (e.line) { auto* l = qgraphicsitem_cast<QGraphicsLineItem*>(e.line);
            if (l) l->setPen(QPen(QColor(0x55,0x66,0x88,160),1.6)); }
    }
}

void DagScene::highlightTask(int taskId)
{
    clearHighlight();
    m_highlighted = taskId;
    if (!m_nodes.contains(taskId)) return;
    m_nodes[taskId].ellipse->setPen(QPen(QColor(0xFF,0xD7,0x00), 3.5));
}

void DagScene::clearHighlight()
{
    if (m_highlighted >= 0 && m_nodes.contains(m_highlighted)) {
        int id = m_highlighted;
        QColor rim = m_taskVm.contains(id) ? getVMColor(m_taskVm[id]).lighter(160)
                                            : getDefaultTaskColor(id).lighter(140);
        m_nodes[id].ellipse->setPen(QPen(rim, m_criticalPath.contains(id) ? 3.0 : 2.0));
    }
    m_highlighted = -1;
}

void DagScene::highlightCriticalPath(const QVector<int>& path)
{
    m_criticalPath = QSet<int>(path.begin(), path.end());
    for (int id : path) {
        if (!m_nodes.contains(id)) continue;
        auto& ni = m_nodes[id];
        QColor c = m_taskVm.contains(id) ? getVMColor(m_taskVm[id]) : getDefaultTaskColor(id);
        ni.ellipse->setPen(QPen(QColor(0xFF,0x55,0x55), 3.5));
    }
    // Highlight critical edges
    QSet<int> pathSet(path.begin(), path.end());
    for (auto& e : m_edges) {
        if (pathSet.contains(e.from) && pathSet.contains(e.to)) {
            if (auto* l = qgraphicsitem_cast<QGraphicsLineItem*>(e.line))
                l->setPen(QPen(QColor(0xFF,0x55,0x55,220), 2.5));
        }
    }
}

void DagScene::showStepTask(int taskId)
{
    // Dim all nodes
    for (int id : m_nodes.keys()) {
        auto& ni = m_nodes[id];
        QColor c = getDefaultTaskColor(id);
        ni.ellipse->setBrush(QBrush(QColor(c.red(),c.green(),c.blue(),60)));
    }
    // Brighten scheduled one
    if (m_nodes.contains(taskId)) {
        QColor c = m_taskVm.contains(taskId) ? getVMColor(m_taskVm[taskId])
                                              : getDefaultTaskColor(taskId);
        m_nodes[taskId].ellipse->setBrush(QBrush(c));
        m_nodes[taskId].ellipse->setPen(QPen(QColor(0xFF,0xD7,0x00),3));
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────
void DagScene::setNodeColor(int taskId, QColor fill, QColor rim, double glowAlpha)
{
    if (!m_nodes.contains(taskId)) return;
    auto& ni = m_nodes[taskId];
    ni.ellipse->setBrush(QBrush(fill));
    ni.ellipse->setPen(QPen(rim, m_criticalPath.contains(taskId) ? 3.5 : 2.0));
    if (ni.glow) {
        QColor g = fill; g.setAlphaF(glowAlpha);
        ni.glow->setBrush(QBrush(g));
    }
}

QVector<QVector<int>> DagScene::computeLevels() const
{
    int n = m_dag.tasks.size();
    QVector<int> indeg(n,0), lvl(n,0);
    for (const auto& t : m_dag.tasks) for (int s : t.successors) indeg[s]++;
    QVector<int> q, order;
    for (int i=0;i<n;i++) if(!indeg[i]) q.push_back(i);
    while(!q.empty()){
        std::sort(q.begin(),q.end());
        int cur=q.front(); q.erase(q.begin()); order.push_back(cur);
        for(int s:m_dag.tasks[cur].successors) if(!--indeg[s]) q.push_back(s);
    }
    for(int id:order) for(int s:m_dag.tasks[id].successors) lvl[s]=std::max(lvl[s],lvl[id]+1);
    int mx=*std::max_element(lvl.begin(),lvl.end());
    QVector<QVector<int>> layers(mx+1);
    for(int i=0;i<n;i++) layers[lvl[i]].push_back(i);
    return layers;
}

void DagScene::drawEdge(QPointF from, QPointF to, bool isCritical)
{
    QPointF dir = to - from;
    double len = std::sqrt(dir.x()*dir.x()+dir.y()*dir.y());
    if (len<1e-6) return;
    dir /= len;
    QPointF pFrom = from + dir*R, pTo = to - dir*R;
    QColor edgeC = isCritical ? QColor(0xFF,0x55,0x55,220) : QColor(0x55,0x77,0xAA,160);
    float lw = isCritical ? 2.5f : 1.6f;
    auto* line = addLine(QLineF(pFrom,pTo), QPen(edgeC,lw,Qt::SolidLine,Qt::RoundCap));
    line->setZValue(-1);
    // arrowhead
    double al=12,aw=5;
    QPointF lt = pTo-dir*al+QPointF(-dir.y(),dir.x())*aw;
    QPointF rt = pTo-dir*al-QPointF(-dir.y(),dir.x())*aw;
    QPolygonF tri; tri<<pTo<<lt<<rt;
    auto* arr = addPolygon(tri, QPen(Qt::NoPen), QBrush(edgeC));
    arr->setZValue(-1);
    EdgeItem ei; ei.from=-1; ei.to=-1; ei.line=line; ei.arrow=arr;
    m_edges.push_back(ei);
    m_edges.back().from = (int)(from.x()/H_GAP); // store approximate
}

void DagScene::buildLayout()
{
    auto layers = computeLevels();
    QFont labelFont("Segoe UI", 9, QFont::Bold);

    // Compute node positions
    QMap<int,QPointF> pos;
    for (int li=0; li<layers.size(); li++) {
        const auto& lyr = layers[li];
        int cnt = lyr.size();
        for (int pi=0; pi<cnt; pi++) {
            int tid = lyr[pi];
            double x = li * H_GAP;
            double y = (pi - (cnt-1)/2.0) * V_GAP;
            pos[tid] = {x,y};
        }
    }

    // Draw edges first (z = -1)
    for (const auto& task : m_dag.tasks) {
        for (int s : task.successors) {
            if (pos.contains(task.id) && pos.contains(s)) {
                drawEdge(pos[task.id], pos[s]);
            }
        }
    }

    // Draw nodes
    for (int ti=0; ti<m_dag.tasks.size(); ti++) {
        const auto& task = m_dag.tasks[ti];
        QPointF p = pos[ti];
        QColor base = getDefaultTaskColor(ti);
        QColor rim  = base.lighter(140);

        // Build tooltip: exec times per VM
        QString tip = QString("<b>%1</b><br>").arg(task.name);
        tip += "<table>";
        for (int v=0; v<m_dag.vms.size(); v++)
            tip += QString("<tr><td>%1:</td><td>&nbsp;%.2f</td></tr>")
                   .arg(m_dag.vms[v].name)
                   .arg(task.execTimes[v]);
        tip += "</table>";
        if (!task.predecessors.isEmpty()) {
            tip += "<br><b>Preds:</b> ";
            for (int p : task.predecessors) tip += QString("T%1 ").arg(p);
        }
        if (!task.successors.isEmpty()) {
            tip += "<br><b>Succs:</b> ";
            for (int s : task.successors) tip += QString("T%1 ").arg(s);
        }

        // Glow
        double gr = R+8;
        auto* glow = addEllipse(p.x()-gr,p.y()-gr,gr*2,gr*2,
                                QPen(Qt::NoPen),
                                QBrush(QColor(base.red(),base.green(),base.blue(),35)));
        glow->setZValue(-2);

        // Circle
        auto* ell = addEllipse(p.x()-R,p.y()-R,R*2,R*2,
                               QPen(rim,2),QBrush(base));
        ell->setZValue(1);
        ell->setToolTip(tip);
        ell->setData(0, ti); // store task id

        // Label
        auto* lbl = addText(task.name, labelFont);
        lbl->setDefaultTextColor(Qt::white);
        QRectF br = lbl->boundingRect();
        lbl->setPos(p.x()-br.width()/2, p.y()-br.height()/2);
        lbl->setZValue(2);
        lbl->setToolTip(tip);

        NodeInfo ni; ni.ellipse=ell; ni.label=lbl; ni.glow=glow; ni.center=p;
        m_nodes[ti] = ni;
    }

    setSceneRect(itemsBoundingRect().adjusted(-40,-40,40,40));
}

void DagScene::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    for (auto* item : items(e->scenePos())) {
        if (item->data(0).isValid()) {
            int tid = item->data(0).toInt();
            highlightTask(tid);
            emit taskSelected(tid);
            return;
        }
    }
    QGraphicsScene::mousePressEvent(e);
}

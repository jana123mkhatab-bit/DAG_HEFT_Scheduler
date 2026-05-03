#include "ComparisonPanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QStyleOption>
#include <algorithm>
#include <cmath>

static const char* ALGO_NAMES[] = {"HEFT", "HEFT + DP", "HEFT + D&C"};
static const QColor ALGO_COLORS[] = {
    QColor(0x42,0x8B,0xFF), QColor(0xBD,0x6F,0xFF), QColor(0x2E,0xCC,0xA8)
};

ComparisonPanel::ComparisonPanel(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(110);
    setStyleSheet("background-color:#0E1120; border-top:2px solid #1E2436;");

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(16,8,16,8); outer->setSpacing(0);

    // Title
    auto* titleLbl = new QLabel("Algorithm Comparison", this);
    titleLbl->setStyleSheet("color:#7799CC; font-weight:bold; font-size:12px; border:none;");
    titleLbl->setFixedWidth(160);
    outer->addWidget(titleLbl);

    // One column per algorithm
    for (int i = 0; i < 3; i++) {
        auto* sep = new QFrame(this);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("color:#2A3048;");
        outer->addWidget(sep);

        auto* col = new QVBoxLayout();
        col->setSpacing(2);

        m_rows[i].name = new QLabel(ALGO_NAMES[i], this);
        m_rows[i].name->setStyleSheet(
            QString("color:%1; font-weight:bold; font-size:11px; border:none;")
            .arg(ALGO_COLORS[i].name()));
        col->addWidget(m_rows[i].name);

        m_rows[i].makespan = new QLabel("Makespan: —", this);
        m_rows[i].makespan->setStyleSheet("color:#DDEEFF; font-size:11px; border:none;");
        col->addWidget(m_rows[i].makespan);

        m_rows[i].runtime = new QLabel("Runtime: —", this);
        m_rows[i].runtime->setStyleSheet("color:#99AABB; font-size:10px; border:none;");
        col->addWidget(m_rows[i].runtime);

        m_rows[i].cp = new QLabel("Crit. path: —", this);
        m_rows[i].cp->setStyleSheet("color:#FF9999; font-size:10px; border:none;");
        col->addWidget(m_rows[i].cp);

        outer->addLayout(col, 1);
    }

    // Chart area placeholder (drawn in paintEvent)
    outer->addStretch(1);
}

void ComparisonPanel::setResult(int idx, const AlgorithmResult& r)
{
    if (idx < 0 || idx > 2) return;
    m_results[idx] = r;
    refresh();
    update();
}

void ComparisonPanel::clear()
{
    for (int i=0;i<3;i++) m_results[i]=AlgorithmResult{};
    refresh();
    update();
}

void ComparisonPanel::refresh()
{
    // Find best makespan to highlight
    double best = 1e18;
    for (int i=0;i<3;i++) if (m_results[i].isValid) best=std::min(best,m_results[i].makespan);

    for (int i=0;i<3;i++) {
        if (!m_results[i].isValid) {
            m_rows[i].makespan->setText("Makespan: —");
            m_rows[i].runtime->setText("Runtime: —");
            m_rows[i].cp->setText("Crit. path: —");
        } else {
            bool isBest = std::abs(m_results[i].makespan - best) < 0.01;
            QString msStyle = isBest ? "color:#4CAF50; font-size:12px; font-weight:bold; border:none;"
                                     : "color:#DDEEFF; font-size:11px; border:none;";
            m_rows[i].makespan->setStyleSheet(msStyle);
            m_rows[i].makespan->setText(QString("Makespan: %1 %2")
                .arg(m_results[i].makespan,0,'f',2)
                .arg(isBest ? "★" : ""));
            m_rows[i].runtime->setText(QString("Runtime: %1 ms")
                .arg(m_results[i].runtimeMs,0,'f',3));
            int cpLen = m_results[i].criticalPath.size();
            m_rows[i].cp->setText(QString("Crit. path: %1 tasks").arg(cpLen));
        }
    }
}

void ComparisonPanel::paintEvent(QPaintEvent*)
{
    QStyleOption opt; opt.initFrom(this);
    QPainter p(this); style()->drawPrimitive(QStyle::PE_Widget,&opt,&p,this);

    // Simple horizontal bar chart on right side
    double mx = 0;
    for (int i=0;i<3;i++) if (m_results[i].isValid) mx=std::max(mx,m_results[i].makespan);
    if (mx<=0) return;

    int cw=230, ch=80, cx=width()-cw-20, cy=(height()-ch)/2;
    p.setPen(QPen(QColor(0x33,0x3F,0x55),1));
    p.drawRect(cx,cy,cw,ch);

    int barH=18, gap=6, startY=cy+8;
    for (int i=0;i<3;i++) {
        if (!m_results[i].isValid) continue;
        int bw=(int)((m_results[i].makespan/mx)*(cw-4));
        QColor c=ALGO_COLORS[i]; c.setAlpha(200);
        p.fillRect(cx+2, startY+i*(barH+gap), bw, barH, c);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI",8));
        p.drawText(cx+2, startY+i*(barH+gap)-1, cw-4, barH,
                   Qt::AlignVCenter|Qt::AlignLeft,
                   QString(" %1: %2").arg(ALGO_NAMES[i])
                   .arg(m_results[i].makespan,0,'f',1));
    }
}

#include "ControlPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QScrollArea>
#include <QSplitter>

// ─── Helpers ──────────────────────────────────────────────────────────────────
static QGroupBox* makeGroup(const QString& title, QWidget* parent)
{
    auto* g = new QGroupBox(title, parent);
    g->setStyleSheet(
        "QGroupBox {"
        "  color: #8AADCC; font-weight: bold; font-size: 11px;"
        "  border: 1px solid #2C3555; border-radius: 6px;"
        "  margin-top: 14px; padding-top: 4px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 5px;"
        "}");
    return g;
}

static QPushButton* makeBtn(const QString& text, const QString& bg, QWidget* parent)
{
    auto* b = new QPushButton(text, parent);
    b->setMinimumHeight(32);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    b->setStyleSheet(QString(
        "QPushButton { background:%1; color:white; font-weight:bold; font-size:11px;"
        "  padding:6px 10px; border-radius:5px; border:none; }"
        "QPushButton:hover  { background:%2; }"
        "QPushButton:pressed{ background:%3; }"
        "QPushButton:disabled{ background:#2A2A3A; color:#555; }")
        .arg(bg)
        .arg(QColor(bg).lighter(120).name())
        .arg(QColor(bg).darker(110).name()));
    return b;
}

static QLabel* makeLbl(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setStyleSheet("color:#99BBCC; font-size:10px; border:none;");
    return l;
}

// ─── Constructor ─────────────────────────────────────────────────────────────
ControlPanel::ControlPanel(QWidget* parent) : QWidget(parent)
{
    setFixedWidth(280);
    setStyleSheet("background-color:#131724;");

    // ── Outermost layout: title | splitter(scroll controls + log) ─────────────
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── App title ─────────────────────────────────────────────────────────────
    auto* titleWidget = new QWidget(this);
    titleWidget->setFixedHeight(56);
    titleWidget->setStyleSheet("background:#0D1020; border-bottom:1px solid #1E2640;");
    auto* titleL = new QVBoxLayout(titleWidget);
    titleL->setContentsMargins(14, 10, 14, 10);
    titleL->setSpacing(2);
    auto* t1 = new QLabel("☁  Cloud Scheduling", titleWidget);
    t1->setStyleSheet("color:#CCDDFF; font-size:14px; font-weight:bold; border:none;");
    auto* t2 = new QLabel("HEFT  ·  DP  ·  D&C  Visualizer", titleWidget);
    t2->setStyleSheet("color:#5577AA; font-size:10px; border:none;");
    titleL->addWidget(t1);
    titleL->addWidget(t2);
    rootLayout->addWidget(titleWidget);

    // ── Vertical splitter: scroll-area (controls) on top, log on bottom ───────
    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(4);
    splitter->setStyleSheet(
        "QSplitter::handle { background:#1E2640; }"
        "QSplitter::handle:hover { background:#2C3A58; }");

    // ────────────────────── SCROLL AREA (all controls) ───────────────────────
    auto* scrollArea = new QScrollArea(splitter);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(
        "QScrollArea { background:#131724; border:none; }"
        "QScrollBar:vertical { background:#131724; width:6px; margin:0; }"
        "QScrollBar::handle:vertical { background:#2C3555; border-radius:3px; min-height:20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }");

    auto* content = new QWidget();
    content->setStyleSheet("background:#131724;");
    auto* cl = new QVBoxLayout(content);
    cl->setContentsMargins(10, 12, 10, 12);
    cl->setSpacing(14);

    const QString spinStyle =
        "QSpinBox { background:#1A2038; color:white; border:1px solid #374060;"
        "  padding:4px 6px; border-radius:4px; font-size:11px; }"
        "QSpinBox::up-button, QSpinBox::down-button { width:18px; }";

    // ── Group 1: DAG Generation ───────────────────────────────────────────────
    {
        auto* g = makeGroup("DAG Generation", content);
        auto* gl = new QVBoxLayout(g);
        gl->setContentsMargins(10, 16, 10, 10);
        gl->setSpacing(8);

        auto addRow = [&](const QString& lbl, QSpinBox* spin) {
            auto* row = new QHBoxLayout();
            row->setSpacing(10);
            auto* lb = makeLbl(lbl, content);
            lb->setFixedWidth(46);
            row->addWidget(lb);
            row->addWidget(spin, 1);
            gl->addLayout(row);
        };

        m_taskSpin = new QSpinBox(content);
        m_taskSpin->setRange(4, 30); m_taskSpin->setValue(10);
        m_taskSpin->setStyleSheet(spinStyle);

        m_vmSpin = new QSpinBox(content);
        m_vmSpin->setRange(2, 8); m_vmSpin->setValue(4);
        m_vmSpin->setStyleSheet(spinStyle);

        m_seedSpin = new QSpinBox(content);
        m_seedSpin->setRange(0, 9999); m_seedSpin->setValue(42);
        m_seedSpin->setStyleSheet(spinStyle);

        addRow("Tasks:", m_taskSpin);
        addRow("VMs:",   m_vmSpin);
        addRow("Seed:",  m_seedSpin);

        gl->addSpacing(2);
        m_genBtn = makeBtn("⚡  Generate DAG", "#2E7D32", content);
        gl->addWidget(m_genBtn);
        cl->addWidget(g);
    }

    // ── Group 2: Run Algorithms ───────────────────────────────────────────────
    {
        auto* g = makeGroup("Run Algorithms", content);
        auto* gl = new QVBoxLayout(g);
        gl->setContentsMargins(10, 16, 10, 10);
        gl->setSpacing(8);

        m_heftBtn = makeBtn("▶  Run HEFT",         "#1565C0", content);
        m_dpBtn   = makeBtn("▶  Run HEFT + DP",    "#6A1B9A", content);
        m_dcBtn   = makeBtn("▶  Run HEFT + D&C",   "#00695C", content);
        gl->addWidget(m_heftBtn);
        gl->addWidget(m_dpBtn);
        gl->addWidget(m_dcBtn);
        cl->addWidget(g);
    }

    // ── Group 3: View Toggle ──────────────────────────────────────────────────
    {
        auto* g = makeGroup("View Algorithm Result", content);
        auto* gl = new QVBoxLayout(g);
        gl->setContentsMargins(10, 16, 10, 10);
        gl->setSpacing(6);

        m_algoToggle = new QComboBox(content);
        m_algoToggle->addItems({"HEFT", "HEFT + Dynamic Programming", "HEFT + Divide & Conquer"});
        m_algoToggle->setStyleSheet(
            "QComboBox { background:#1A2038; color:white; border:1px solid #374060;"
            "  padding:5px 8px; border-radius:4px; font-size:11px; }"
            "QComboBox::drop-down { width:22px; }"
            "QComboBox QAbstractItemView { background:#1A2038; color:white; border:1px solid #374060; }");
        gl->addWidget(m_algoToggle);
        cl->addWidget(g);
    }

    // ── Group 4: Step-by-Step ─────────────────────────────────────────────────
    {
        auto* g = makeGroup("Step-by-Step Execution", content);
        auto* gl = new QVBoxLayout(g);
        gl->setContentsMargins(10, 16, 10, 10);
        gl->setSpacing(8);

        m_stepLbl = new QLabel("Step: — / —", content);
        m_stepLbl->setAlignment(Qt::AlignCenter);
        m_stepLbl->setStyleSheet(
            "color:#AABBCC; font-size:11px; border:none;"
            "background:#1A2038; border-radius:4px; padding:4px 8px;");
        gl->addWidget(m_stepLbl);

        auto* row = new QHBoxLayout();
        row->setSpacing(6);
        m_stepReset = makeBtn("⏮", "#2A3248", content);
        m_stepPrev  = makeBtn("◀", "#2A3248", content);
        m_stepNext  = makeBtn("▶", "#1565C0", content);
        for (auto* b : {m_stepReset, m_stepPrev, m_stepNext})
            b->setMinimumHeight(34);
        row->addWidget(m_stepReset);
        row->addWidget(m_stepPrev);
        row->addWidget(m_stepNext);
        gl->addLayout(row);
        cl->addWidget(g);
    }

    // ── Group 5: Export ───────────────────────────────────────────────────────
    {
        auto* g = makeGroup("Export", content);
        auto* gl = new QVBoxLayout(g);
        gl->setContentsMargins(10, 16, 10, 10);
        gl->setSpacing(8);

        m_expGantt = makeBtn("🖼  Export Gantt (PNG)",    "#37474F", content);
        m_expCSV   = makeBtn("📄  Export Results (CSV)",  "#37474F", content);
        gl->addWidget(m_expGantt);
        gl->addWidget(m_expCSV);
        cl->addWidget(g);
    }

    cl->addStretch(1);
    scrollArea->setWidget(content);
    splitter->addWidget(scrollArea);

    // ────────────────────── LOG PANEL (bottom of splitter) ───────────────────
    auto* logPanel = new QWidget(splitter);
    logPanel->setStyleSheet("background:#0C0F18; border-top:1px solid #1E2640;");
    auto* logL = new QVBoxLayout(logPanel);
    logL->setContentsMargins(8, 8, 8, 8);
    logL->setSpacing(4);

    auto* logTitle = new QLabel("Output Log", logPanel);
    logTitle->setStyleSheet("color:#446688; font-size:10px; font-weight:bold; border:none;");
    logL->addWidget(logTitle);

    m_log = new QTextEdit(logPanel);
    m_log->setReadOnly(true);
    m_log->setStyleSheet(
        "QTextEdit { background:#080B14; color:#AABBCC;"
        "  font-family:Consolas,monospace; font-size:10px;"
        "  border:1px solid #1A2030; border-radius:3px; }");
    logL->addWidget(m_log);

    splitter->addWidget(logPanel);

    // Give the controls ~70% and log ~30% of the space
    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({500, 180});

    rootLayout->addWidget(splitter, 1);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_genBtn, &QPushButton::clicked, this, [this] {
        emit generateRequested(m_taskSpin->value(), m_vmSpin->value(),
                               (unsigned)m_seedSpin->value());
    });
    connect(m_heftBtn,  &QPushButton::clicked, this, &ControlPanel::runHEFTRequested);
    connect(m_dpBtn,    &QPushButton::clicked, this, &ControlPanel::runDPRequested);
    connect(m_dcBtn,    &QPushButton::clicked, this, &ControlPanel::runDCRequested);
    connect(m_expGantt, &QPushButton::clicked, this, &ControlPanel::exportGanttRequested);
    connect(m_expCSV,   &QPushButton::clicked, this, &ControlPanel::exportCSVRequested);
    connect(m_stepNext,  &QPushButton::clicked, this, &ControlPanel::stepForwardRequested);
    connect(m_stepPrev,  &QPushButton::clicked, this, &ControlPanel::stepBackwardRequested);
    connect(m_stepReset, &QPushButton::clicked, this, &ControlPanel::stepResetRequested);
    connect(m_algoToggle, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ControlPanel::activeAlgorithmChanged);
}

// ─── Public methods ───────────────────────────────────────────────────────────
void ControlPanel::appendLog(const QString& msg, const QString& color)
{
    m_log->append(QString("<span style='color:%1'>%2</span>")
                  .arg(color, msg.toHtmlEscaped()));
}

void ControlPanel::clearLog()
{
    m_log->clear();
}

void ControlPanel::setStepBounds(int total)
{
    m_stepLbl->setText(QString("Step: 0 / %1").arg(total));
}

void ControlPanel::updateStepLabel(int current, int total)
{
    m_stepLbl->setText(QString("Step: %1 / %2").arg(current).arg(total));
}

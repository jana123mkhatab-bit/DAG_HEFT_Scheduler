#pragma once
#include <QWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QSlider>
#include "DataTypes.h"

class ControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ControlPanel(QWidget* parent = nullptr);
    void appendLog(const QString& msg, const QString& color="#AABBCC");
    void clearLog();
    void setStepBounds(int total);
    void updateStepLabel(int current, int total);

signals:
    void generateRequested(int numTasks, int numVMs, unsigned seed);
    void runHEFTRequested();
    void runDPRequested();
    void runDCRequested();
    void exportGanttRequested();
    void exportCSVRequested();
    void stepForwardRequested();
    void stepBackwardRequested();
    void stepResetRequested();
    void activeAlgorithmChanged(int idx); // 0=HEFT, 1=DP, 2=D&C

private:
    QSpinBox*    m_taskSpin;
    QSpinBox*    m_vmSpin;
    QSpinBox*    m_seedSpin;
    QPushButton* m_genBtn;
    QPushButton* m_heftBtn;
    QPushButton* m_dpBtn;
    QPushButton* m_dcBtn;
    QComboBox*   m_algoToggle;
    QPushButton* m_stepPrev;
    QPushButton* m_stepNext;
    QPushButton* m_stepReset;
    QLabel*      m_stepLbl;
    QPushButton* m_expGantt;
    QPushButton* m_expCSV;
    QTextEdit*   m_log;
};

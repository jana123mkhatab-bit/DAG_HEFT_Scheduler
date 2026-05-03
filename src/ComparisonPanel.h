#pragma once
#include <QWidget>
#include <QLabel>
#include "DataTypes.h"

class ComparisonPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ComparisonPanel(QWidget* parent = nullptr);
    void setResult(int algoIdx, const AlgorithmResult& r); // 0=HEFT,1=DP,2=D&C
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void refresh();

    AlgorithmResult m_results[3];

    // Per-algorithm labels
    struct AlgoRow { QLabel* name; QLabel* makespan; QLabel* runtime; QLabel* cp; };
    AlgoRow m_rows[3];
};

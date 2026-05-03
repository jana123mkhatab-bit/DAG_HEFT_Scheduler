#pragma once
#include "DataTypes.h"

class AlgorithmEngine
{
public:
    // ── DAG generation ────────────────────────────────────────────────────────
    // numTasks: 4-30, numVMs: 2-8
    static DAGData generateDAG(int numTasks, int numVMs,
                               double commFactor = 0.5,
                               unsigned seed = 42);

    // ── Three scheduling algorithms ───────────────────────────────────────────
    static AlgorithmResult runHEFT      (const DAGData& dag);
    static AlgorithmResult runHEFT_DP   (const DAGData& dag);  // + level-wise DP
    static AlgorithmResult runHEFT_DC   (const DAGData& dag);  // + Divide & Conquer

    // ── Utilities ─────────────────────────────────────────────────────────────
    static QVector<int>    topologicalSort (const DAGData& dag);
    static QVector<QVector<int>> computeLevels(const DAGData& dag);
    static QVector<int>    traceCriticalPath(const AlgorithmResult& res,
                                             const DAGData& dag);

private:
    static QVector<double> computeUpwardRank  (const DAGData& dag);
    static QVector<double> computeDownwardRank(const DAGData& dag);

    static double commCost(const DAGData& dag,
                           int predTask, int predVm,
                           int succTask, int succVm);

    static double computeEFT(const DAGData& dag,
                             int taskId, int vmId,
                             const QVector<double>&          vmReady,
                             const QMap<int,ScheduleEntry>&  scheduled,
                             double& estOut);


};

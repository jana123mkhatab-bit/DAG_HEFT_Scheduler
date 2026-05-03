#include "AlgorithmEngine.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>
#include <limits>
#include <functional>
#include <cmath>

static constexpr double INF = std::numeric_limits<double>::max();

// ─── Communication cost ───────────────────────────────────────────────────────
double AlgorithmEngine::commCost(const DAGData& dag,
                                  int predTask, int predVm,
                                  int /*succTask*/, int succVm)
{
    if (predVm == succVm) return 0.0;
    const auto& et = dag.tasks[predTask].execTimes;
    double avg = 0.0; for (double v : et) avg += v;
    avg /= et.size();
    return dag.commCostFactor * avg;
}

// ─── EFT ─────────────────────────────────────────────────────────────────────
double AlgorithmEngine::computeEFT(const DAGData& dag, int taskId, int vmId,
                                    const QVector<double>& vmReady,
                                    const QMap<int,ScheduleEntry>& scheduled,
                                    double& estOut)
{
    double est = vmReady[vmId];
    for (int p : dag.tasks[taskId].predecessors) {
        if (!scheduled.contains(p)) continue;
        const auto& se = scheduled[p];
        est = std::max(est, se.finishTime + commCost(dag, p, se.vmId, taskId, vmId));
    }
    estOut = est;
    return est + dag.tasks[taskId].execTimes[vmId];
}

// ─── Topological sort (Kahn) ──────────────────────────────────────────────────
QVector<int> AlgorithmEngine::topologicalSort(const DAGData& dag)
{
    int n = dag.tasks.size();
    QVector<int> indeg(n, 0);
    for (const auto& t : dag.tasks) for (int s : t.successors) indeg[s]++;
    QVector<int> q, res;
    for (int i = 0; i < n; i++) if (!indeg[i]) q.push_back(i);
    while (!q.empty()) {
        std::sort(q.begin(), q.end());
        int cur = q.front(); q.erase(q.begin()); res.push_back(cur);
        for (int s : dag.tasks[cur].successors) if (!--indeg[s]) q.push_back(s);
    }
    return res;
}

// ─── Topological levels ───────────────────────────────────────────────────────
QVector<QVector<int>> AlgorithmEngine::computeLevels(const DAGData& dag)
{
    int n = dag.tasks.size();
    QVector<int> lvl(n, 0);
    for (int id : topologicalSort(dag))
        for (int s : dag.tasks[id].successors)
            lvl[s] = std::max(lvl[s], lvl[id] + 1);
    int mx = *std::max_element(lvl.begin(), lvl.end());
    QVector<QVector<int>> layers(mx + 1);
    for (int i = 0; i < n; i++) layers[lvl[i]].push_back(i);
    return layers;
}

// ─── Upward rank ──────────────────────────────────────────────────────────────
QVector<double> AlgorithmEngine::computeUpwardRank(const DAGData& dag)
{
    int n = dag.tasks.size();
    QVector<double> rank(n, -1.0);
    auto avg = [&](int id){ double s=0; for(double v:dag.tasks[id].execTimes) s+=v; return s/dag.tasks[id].execTimes.size(); };
    std::function<double(int)> calc = [&](int id) -> double {
        if (rank[id] >= 0) return rank[id];
        double best = 0;
        for (int s : dag.tasks[id].successors)
            best = std::max(best, commCost(dag,id,0,s,1) + calc(s));
        return rank[id] = avg(id) + best;
    };
    for (int i = 0; i < n; i++) calc(i);
    return rank;
}

// ─── Downward rank ────────────────────────────────────────────────────────────
QVector<double> AlgorithmEngine::computeDownwardRank(const DAGData& dag)
{
    int n = dag.tasks.size();
    QVector<double> rank(n, -1.0);
    auto avg = [&](int id){ double s=0; for(double v:dag.tasks[id].execTimes) s+=v; return s/dag.tasks[id].execTimes.size(); };
    std::function<double(int)> calc = [&](int id) -> double {
        if (rank[id] >= 0) return rank[id];
        double best = 0;
        for (int p : dag.tasks[id].predecessors)
            best = std::max(best, calc(p) + avg(p) + commCost(dag,p,0,id,1));
        return rank[id] = best;
    };
    for (int i = 0; i < n; i++) calc(i);
    return rank;
}

// ─── Critical path tracing ────────────────────────────────────────────────────
QVector<int> AlgorithmEngine::traceCriticalPath(const AlgorithmResult& res, const DAGData& dag)
{
    QMap<int,ScheduleEntry> sched;
    for (const auto& se : res.entries) sched[se.taskId] = se;
    // Find exit task (max finish)
    int exitTask = -1; double maxFT = -1;
    for (auto it = sched.begin(); it != sched.end(); ++it)
        if (it->finishTime > maxFT) { maxFT = it->finishTime; exitTask = it->taskId; }
    // Trace back
    QVector<int> path;
    int cur = exitTask;
    while (cur >= 0) {
        path.prepend(cur);
        const auto& se = sched[cur];
        int critPred = -1; double maxContrib = -1;
        for (int p : dag.tasks[cur].predecessors) {
            if (!sched.contains(p)) continue;
            const auto& pse = sched[p];
            double cc = commCost(dag, p, pse.vmId, cur, se.vmId);
            double contrib = pse.finishTime + cc;
            if (contrib > maxContrib) { maxContrib = contrib; critPred = p; }
        }
        cur = critPred;
    }
    return path;
}

// ═══ HEFT (standard) ══════════════════════════════════════════════════════════
AlgorithmResult AlgorithmEngine::runHEFT(const DAGData& dag)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    int n = dag.tasks.size(), m = dag.numVMs();
    auto upRank = computeUpwardRank(dag);
    QVector<int> priority(n); std::iota(priority.begin(), priority.end(), 0);
    std::sort(priority.begin(), priority.end(), [&](int a, int b){ return upRank[a]>upRank[b]; });

    QVector<double> vmReady(m, 0.0);
    QMap<int,ScheduleEntry> scheduled;
    for (int tid : priority) {
        int bestVM=0; double bestEFT=INF, bestEST=0;
        for (int v = 0; v < m; v++) {
            double est=0, eft=computeEFT(dag,tid,v,vmReady,scheduled,est);
            if (eft<bestEFT) { bestEFT=eft; bestVM=v; bestEST=est; }
        }
        scheduled[tid] = {tid, bestVM, bestEST, bestEFT};
        vmReady[bestVM] = bestEFT;
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    AlgorithmResult res;
    res.algorithmName = "HEFT";
    res.algorithmDesc = "Heterogeneous Earliest Finish Time — greedy upward-rank priority";
    res.numVMs=m; res.numTasks=n;
    for (auto& se : scheduled) res.entries.push_back(se);
    res.makespan = *std::max_element(vmReady.begin(), vmReady.end());
    res.runtimeMs = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.criticalPath = traceCriticalPath(res, dag);
    res.isValid = true;
    return res;
}

// ═══ HEFT + Dynamic Programming (Bilateral Rank + Full Global DP) ═══════════════
//
// Phase 1 — Bilateral rank priority (upRank + downRank, descending)
// Phase 2 — Full n×m DP table: dp[i][v] = min makespan when the i-th task
//           in priority order is placed on VM v, with parent backlinks
// Phase 3 — Replay the optimal assignment to compute exact EST/EFT timestamps
AlgorithmResult AlgorithmEngine::runHEFT_DP(const DAGData& dag)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    int n = dag.tasks.size(), m = dag.numVMs();

    // ── Phase 1: Bilateral rank priority ─────────────────────────────────────
    auto upRank = computeUpwardRank(dag);
    auto dnRank = computeDownwardRank(dag);
    QVector<double> biRank(n);
    for (int i = 0; i < n; ++i)
        biRank[i] = upRank[i] + dnRank[i];

    QVector<int> priority(n);
    std::iota(priority.begin(), priority.end(), 0);
    std::sort(priority.begin(), priority.end(), [&](int a, int b){
     if (biRank[a] != biRank[b]) return biRank[a] > biRank[b];
        // Tie-break: if a is a predecessor of b, a comes first
        for (int succ : dag.tasks[a].successors)
            if (succ == b) return true;
        return a < b;   // stable fallback
    });

    // ── Phase 2: Global DP ────────────────────────────────────────────────────
    // Snapshot carries the state needed to evaluate future tasks
    struct Snapshot {
        QVector<double> vmReady;
        QVector<double> taskFinish;
        QVector<int>    taskVm;
    };
    static constexpr double EDP_INF = std::numeric_limits<double>::max() / 2.0;

    QVector<QVector<double>>   dp  (n, QVector<double>(m, EDP_INF));
    QVector<QVector<int>>      par (n, QVector<int>   (m, -1));

    Snapshot emptySnap;
    emptySnap.vmReady    = QVector<double>(m, 0.0);
    emptySnap.taskFinish = QVector<double>(n, -1.0);
    emptySnap.taskVm     = QVector<int>   (n, -1);

    QVector<QVector<Snapshot>> snaps(n, QVector<Snapshot>(m, emptySnap));

    // EFT computed from a snapshot (read-only, no state mutation)
    auto snapEFT = [&](int tid, int vm, const Snapshot& s, double& estOut) -> double {
        double est = s.vmReady[vm];
        for (int pred : dag.tasks[tid].predecessors) {
            if (s.taskVm[pred] < 0) continue;
            double cc = commCost(dag, pred, s.taskVm[pred], tid, vm);
            est = std::max(est, s.taskFinish[pred] + cc);
        }
        estOut = est;
        return est + dag.tasks[tid].execTimes[vm];
    };

    // Base case: place the first priority task on every VM
    {
        int tid = priority[0];
        for (int v = 0; v < m; ++v) {
            double est = 0.0;
            double eft = snapEFT(tid, v, emptySnap, est);
            dp[0][v] = eft;
            snaps[0][v].vmReady[v]      = eft;
            snaps[0][v].taskFinish[tid] = eft;
            snaps[0][v].taskVm[tid]     = v;
        }
    }

    // Forward fill: dp[i][v] = min makespan after tasks 0..i with task i on VM v
    for (int i = 1; i < n; ++i) {
        int tid = priority[i];
        for (int pv = 0; pv < m; ++pv) {
            if (dp[i-1][pv] >= EDP_INF) continue;
            const Snapshot& ps = snaps[i-1][pv];
            for (int v = 0; v < m; ++v) {
                double est = 0.0;
                double eft = snapEFT(tid, v, ps, est);
                double cand = std::max(dp[i-1][pv], eft);
                if (cand < dp[i][v]) {
                    dp[i][v]                  = cand;
                    par[i][v]                 = pv;
                    snaps[i][v]               = ps;
                    snaps[i][v].vmReady[v]    = eft;
                    snaps[i][v].taskFinish[tid] = eft;
                    snaps[i][v].taskVm[tid]   = v;
                }
            }
        }
    }

    // Pick optimal final VM and backtrack to recover assignment
    int bestV = 0; double bestMS = EDP_INF;
    for (int v = 0; v < m; ++v)
        if (dp[n-1][v] < bestMS) { bestMS = dp[n-1][v]; bestV = v; }

    QVector<int> assignment(n);
    assignment[n-1] = bestV;
    for (int i = n-1; i > 0; --i)
        assignment[i-1] = par[i][assignment[i]];

    // ── Phase 3: Replay to compute exact EST / EFT timestamps ────────────────
    QVector<double>         vmReady(m, 0.0);
    QMap<int,ScheduleEntry> scheduled;
    for (int i = 0; i < n; ++i) {
        int tid = priority[i], vm = assignment[i];
        double est = 0.0;
        double eft = computeEFT(dag, tid, vm, vmReady, scheduled, est);
        scheduled[tid] = {tid, vm, est, eft};
        vmReady[vm]    = eft;
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    AlgorithmResult res;
    res.algorithmName = "HEFT + Dynamic Programming";
    res.algorithmDesc = "Bilateral rank + Full Global DP VM Selection  |  O(n x m^2)";
    res.numVMs = m; res.numTasks = n;
    for (auto& se : scheduled) res.entries.push_back(se);
    res.makespan     = *std::max_element(vmReady.begin(), vmReady.end());
    res.runtimeMs    = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.criticalPath = traceCriticalPath(res, dag);
    res.isValid      = true;
    return res;
}

// ═══ HEFT + Divide & Conquer (BFS Level-Based Clustering) ════════════════════
//
// STEP 1 — DIVIDE:   BFS topological levelling partitions the DAG into
//                    independent layers (tasks in the same layer have no
//                    dependency on each other).
// STEP 2 — CONQUER:  Each layer is scheduled independently: tasks sorted by
//                    descending average execution time, assigned to the
//                    locally fastest VM.
// STEP 3 — MERGE:    Partial schedules combined. For each task, the global
//                    start time respects both VM availability and all
//                    predecessor finish times (+ communication cost when
//                    predecessor and successor run on different VMs).
AlgorithmResult AlgorithmEngine::runHEFT_DC(const DAGData& dag)
{
    auto t0 = std::chrono::high_resolution_clock::now();
    int n = dag.tasks.size(), m = dag.numVMs();

    // ── STEP 1: DIVIDE — BFS topological levels ───────────────────────────────
    auto levels = computeLevels(dag);
    int  L      = static_cast<int>(levels.size());

    auto avgExec = [&](int taskId) -> double {
        double sum = 0.0;
        for (double t : dag.tasks[taskId].execTimes) sum += t;
        return sum / static_cast<double>(dag.tasks[taskId].execTimes.size());
    };

    // ── STEP 2: CONQUER — schedule each level independently ──────────────────
    struct LocalAssign { int taskId; int vmId; };
    QVector<QVector<LocalAssign>> clusterSched(L);

    for (int l = 0; l < L; ++l) {
        QVector<int> cluster = levels[l];
        // Heavier tasks (larger avg exec) scheduled first
        std::sort(cluster.begin(), cluster.end(),
                  [&](int a, int b){ return avgExec(a) > avgExec(b); });

        QVector<double> localFree(m, 0.0);
        for (int u : cluster) {
            int bestVM = 0; double bestFin = INF;
            for (int v = 0; v < m; ++v) {
                double fin = localFree[v] + dag.tasks[u].execTimes[v];
                if (fin < bestFin) { bestFin = fin; bestVM = v; }
            }
            localFree[bestVM] = bestFin;
            clusterSched[l].push_back({u, bestVM});
        }
    }

    // ── STEP 3: MERGE — global schedule with dependency + comm cost ───────────
    QVector<double>         globalFree(m, 0.0);
    QVector<double>         taskFinish(n, 0.0);
    QVector<int>            taskVM    (n, -1);
    QMap<int,ScheduleEntry> scheduled;

    for (int l = 0; l < L; ++l) {
        for (const auto& a : clusterSched[l]) {
            int u = a.taskId, v = a.vmId;

            // Ready time = max over predecessors of (finish + comm cost)
            double ready = 0.0;
            for (int pred : dag.tasks[u].predecessors) {
                if (taskVM[pred] < 0) continue; // predecessor not yet placed (shouldn't occur)
                double cc = commCost(dag, pred, taskVM[pred], u, v);
                ready = std::max(ready, taskFinish[pred] + cc);
            }

            double start  = std::max(globalFree[v], ready);
            double finish = start + dag.tasks[u].execTimes[v];

            globalFree[v] = finish;
            taskFinish[u] = finish;
            taskVM[u]     = v;
            scheduled[u]  = {u, v, start, finish};
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    AlgorithmResult res;
    res.algorithmName = "HEFT + Divide & Conquer";
    res.algorithmDesc = "Level-based clustering with communication-aware merging";
    res.numVMs = m; res.numTasks = n;
    for (auto& se : scheduled) res.entries.push_back(se);
    res.makespan     = *std::max_element(globalFree.begin(), globalFree.end());
    res.runtimeMs    = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.criticalPath = traceCriticalPath(res, dag);
    res.isValid      = true;
    return res;
}

// ═══ DAG Generation (random, variable size) ═══════════════════════════════════
DAGData AlgorithmEngine::generateDAG(int numTasks, int numVMs,
                                      double commFactor, unsigned seed)
{
    std::mt19937 rng(seed);
    auto randExec = [&](double speedFactor) {
        std::uniform_real_distribution<double> d(5.0, 20.0);
        return d(rng) / speedFactor;
    };

    DAGData dag;
    dag.commCostFactor = commFactor;
    dag.label = QString("Random (%1 tasks, %2 VMs)").arg(numTasks).arg(numVMs);

    // Build VMs with varying speeds
    static const double speeds[] = {1.0, 0.8, 1.2, 0.9, 1.5, 0.7, 1.1, 1.3};
    dag.vms.resize(numVMs);
    for (int v = 0; v < numVMs; v++) {
        dag.vms[v] = {v, QString("VM %1").arg(v+1), speeds[v % 8]};
    }

    // Build tasks
    dag.tasks.resize(numTasks);
    for (int i = 0; i < numTasks; i++) {
        dag.tasks[i].id   = i;
        dag.tasks[i].name = QString("T%1").arg(i);
        dag.tasks[i].execTimes.resize(numVMs);
        for (int v = 0; v < numVMs; v++)
            dag.tasks[i].execTimes[v] = randExec(dag.vms[v].speedFactor);
    }

    // Build edges: layered DAG to guarantee no cycles
    int numLayers = std::max(3, numTasks / 4);
    QVector<QVector<int>> layers(numLayers);
    std::uniform_int_distribution<int> layerDist(0, numLayers-2);
    layers[0].push_back(0);
    for (int i = 1; i < numTasks-1; i++) layers[layerDist(rng)+1].push_back(i);
    layers[numLayers-1].push_back(numTasks-1);

    // Remove empty layers
    layers.erase(std::remove_if(layers.begin(), layers.end(),
                                [](const QVector<int>& l){ return l.isEmpty(); }), layers.end());

    // Add edges: each task connects to 1-3 tasks in the next layer
    std::uniform_int_distribution<int> edgeDist(1, 3);
    for (int li = 0; li+1 < layers.size(); li++) {
        for (int src : layers[li]) {
            auto& nextLayer = layers[li+1];
            int numEdges = std::min(edgeDist(rng), (int)nextLayer.size());
            // Shuffle next layer and pick first numEdges
            QVector<int> targets = nextLayer;
            std::shuffle(targets.begin(), targets.end(), rng);
            for (int e = 0; e < numEdges; e++) {
                int dst = targets[e];
                // Avoid duplicate edges
                if (!dag.tasks[src].successors.contains(dst)) {
                    dag.tasks[src].successors.push_back(dst);
                    dag.tasks[dst].predecessors.push_back(src);
                }
            }
        }
    }
    // Ensure last layer task connects to entry if not already reached
    // Ensure every task is reachable: tasks with no preds in non-first layers
    for (int li = 1; li < layers.size(); li++) {
        for (int tid : layers[li]) {
            if (dag.tasks[tid].predecessors.isEmpty() && !layers[li-1].isEmpty()) {
                int src = layers[li-1][0];
                dag.tasks[src].successors.push_back(tid);
                dag.tasks[tid].predecessors.push_back(src);
            }
        }
    }

    return dag;
}

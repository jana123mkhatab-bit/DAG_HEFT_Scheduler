#pragma once
#include <QString>
#include <QVector>
#include <QColor>
#include <QMap>

// ─── Virtual Machine ─────────────────────────────────────────────────────────
struct VM {
    int     id = 0;
    QString name;
    double  speedFactor = 1.0;   // relative speed (1.0 = baseline)
};

// ─── Task node ───────────────────────────────────────────────────────────────
struct Task {
    int     id = 0;
    QString name;
    QVector<double> execTimes;   // exec time on each VM
    QVector<int>    predecessors;
    QVector<int>    successors;
};

// ─── One scheduled block ─────────────────────────────────────────────────────
struct ScheduleEntry {
    int    taskId     = -1;
    int    vmId       = -1;
    double startTime  =  0.0;
    double finishTime =  0.0;
};

// ─── Full result from one algorithm ──────────────────────────────────────────
struct AlgorithmResult {
    QString              algorithmName;
    QString              algorithmDesc;
    QVector<ScheduleEntry> entries;
    QVector<int>         criticalPath;   // task IDs on critical path
    double makespan   = 0.0;
    double runtimeMs  = 0.0;            // wall-clock algorithm time
    int    numVMs     = 0;
    int    numTasks   = 0;
    bool   isValid    = false;
};

// ─── Full DAG + execution matrix ─────────────────────────────────────────────
struct DAGData {
    QVector<Task> tasks;
    QVector<VM>   vms;
    double commCostFactor = 0.5;
    QString label;
    int numVMs() const { return vms.size(); }
};

// ─── VM colour palette ───────────────────────────────────────────────────────
inline QColor getVMColor(int vmId)
{
    static const QColor pal[] = {
        {0xFF,0x6B,0x6B}, // coral
        {0x4E,0xCD,0xC4}, // teal
        {0xFF,0xBE,0x0B}, // amber
        {0x8B,0x5C,0xF6}, // purple
        {0x06,0xD6,0xA0}, // emerald
        {0xEF,0x47,0x6F}, // rose
        {0x11,0x8A,0xB2}, // ocean
        {0xFF,0xD1,0x66}, // yellow
        {0xA8,0xDA,0xDC}, // sky
        {0xE9,0xC4,0x6A}, // gold
    };
    static const int N = (int)(sizeof(pal)/sizeof(pal[0]));
    return pal[vmId % N];
}

// ─── Default task colour (before scheduling) ──────────────────────────────────
inline QColor getDefaultTaskColor(int taskId)
{
    static const QColor pal[] = {
        {0x74,0xB9,0xFF},{0xA2,0x9B,0xFE},{0x55,0xEF,0xC8},{0xFD,0xCB,0x6E},
        {0xFF,0x78,0x75},{0xE1,0x7A,0xFF},{0x00,0xB8,0x94},{0xFF,0xA5,0x02},
        {0xFD,0x79,0xA8},{0x6C,0x5C,0xE7},{0x00,0xCE,0xC3},{0xD6,0x3F,0x31},
        {0x00,0xB8,0xA9},{0xF8,0xB5,0x00},{0xC7,0x3E,0x1D},{0x44,0xBB,0xB4},
        {0xE8,0x4A,0x5F},{0x35,0x8E,0xBD},{0xFF,0xBE,0x7D},{0x8C,0xD1,0x7D},
    };
    static const int N = (int)(sizeof(pal)/sizeof(pal[0]));
    return pal[taskId % N];
}

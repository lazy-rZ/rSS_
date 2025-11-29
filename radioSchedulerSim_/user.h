#pragma once

struct User {
    int id;
    // we want to model the UE so we add mobility
    double x, y;
    double vx, vy;
    double cqi;
    int buffer;
    double bitsPerRb;
    double thr;
    // added new HARQ state (one process per UE for now)
    bool harqActive = false;
    int harqBits = 0; // size of TB currently in HARQ
    int harqTxCount = 0; // how many times this TB was sent

    // for debug
    char harqStatus = '.';

};
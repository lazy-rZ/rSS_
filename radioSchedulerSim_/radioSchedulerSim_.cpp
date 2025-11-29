#include <iostream> 
#include <cstdlib> 
#include <ctime>
#include <vector>
#include <algorithm>
#include <cmath>
#include "common.h"

void updatePosition(User& u, double dt_sec) {
    u.x += u.vx * dt_sec;
    u.y += u.vy * dt_sec;
}

int main()
{
    std::cout << "rSS_ | Build 0.99\n";
    
    // Initilize users
    std::vector<User> users;

    users.push_back({
        0,           // id
        20.0, 0.0,   // x, y  in meters
        0.0, 0.0,    // vx, vy in meters per second
        0.0,         // cqi 
        10000,       // buffer
        0.0,         // bitsPerRb
        0.0          // thr
    });

    users.push_back({
        1,
        120.0, -50.0,
        -4.0, 3.5,    
        0.0,
        18000,
        0.0,
        0.0
    });

    users.push_back({
        2,
        -150.0, 80.0,
        4.5, -4.5,
        0.0,
        9000,
        0.0,
        0.0
    });

    // MCS table 
    std::vector<McsEntry> mcsTable = {
        { 0.0,  50  },   // QPSK low
        { 4.0,  80  },   // QPSK high
        { 7.0,  150 },   // 16QAM
        {10.0,  250 },   // 64QAM
        {13.0,  350 }    // 256QAM
    };

    // Resource block 
    const int TOTAL_RB = 50;
    std::vector<double> bitsPerRbPerUser(users.size());

    // rbOwenr[rb] = index of UE in 'users'
    std::vector<int> rbOwner(TOTAL_RB);

    // Average throughput for Proportional Fair Scheduler
    std::vector<double> avgThroughput(users.size(), 1.0);

    // Set seed to current time for rand
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // For verifying PF is balancing
    std::vector<double> totalThr(users.size(), 0.0);

    // main loop
    for (int tti = 0; tti < 100; ++tti) {
        
        std::cout << "TTI: " << tti << "\n";

        for (auto& u : users) { 
            //updateCQI(u); // update each users CQI using a random number generator
            updatePosition(u, 0.001); // 1 ms
            updateCQI_physical(u); // updates each users CQI using physical model based on mobility
        };

        // call scheduler
        //scheduleRoundRobin(users, rbOwner); // strict time based fairness and simpicity
        //scheduleMaxCQI(users, rbOwner); // maximize instantaneous totalt throughput

        // Precompute bitsPerRb for each UE in this TTI
        for (std::size_t i = 0; i < users.size(); ++i) {
            bitsPerRbPerUser[i] = getBitsPerRbFromCQI(users[i].cqi, mcsTable);
        };

        schedulePF(users, rbOwner, bitsPerRbPerUser, avgThroughput); // balance throughput and fairness

        // reset per UE throughput 
        std::vector<double> thrPerUser(users.size(), 0.0);

        // Accumulate throughput per UE by walking RBs
        for (int rb = 0; rb < TOTAL_RB; ++rb) {
            int uIndex = rbOwner[rb];
            thrPerUser[uIndex] += bitsPerRbPerUser[uIndex];
        };

        // For debuggin
        for (std::size_t i = 0; i < users.size(); ++i) {
            totalThr[i] += thrPerUser[i];
        }

        // for PF scheduling keep exponential average for throughput
        double alpha = 0.1;
        for (std::size_t i = 0; i < users.size(); ++i) {
            avgThroughput[i] = (1 - alpha) * avgThroughput[i] + alpha * thrPerUser[i];
            avgThroughput[i] = std::max(avgThroughput[i], 1.0);
        }

        // Update buffers
        for (std::size_t i = 0; i < users.size(); ++i) {
            int bits = static_cast<int>(thrPerUser[i]);
            // users[i].buffer = std::max(0, users[i].buffer - bits); OLD
            int delivered = runHarqForUser(users[i], bits, users[i].cqi);
            thrPerUser[i] = delivered;

        };

        // clean out print
        for (std::size_t i = 0; i < users.size(); ++i) {
            const auto& u = users[i];
            std::cout << "  UE" << u.id
                << "  CQI=" << u.cqi
                << "  bitsPerRB=" << bitsPerRbPerUser[i]
                << "  THR=" << thrPerUser[i]
                << "  BUF=" << u.buffer
                << "  HARQ=" << u.harqStatus << "\n";

        }

        // Add new traffic arrivals
        for (auto& u : users) {
            u.buffer += static_cast<int>(5000.0 * std::rand() / double(RAND_MAX));
        }
        
    }

    // The totals per UE should be in the same ballpark
    std::cout << "\nTotal throughput over all TTIs:\n";
    for (std::size_t i = 0; i < users.size(); ++i) {
        std::cout << "  UE" << users[i].id
            << " totalThr = " << totalThr[i] << " bits\n";
    }

    return 0;
}


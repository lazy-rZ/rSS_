#include <vector>
#include "user.h"

void scheduleRoundRobin(const std::vector<User>& users, std::vector<int>& rbOwner)
{
    int numUsers = static_cast<int>(users.size());

    for (int rb = 0; rb < static_cast<int>(rbOwner.size()); ++rb) {
        rbOwner[rb] = rb % numUsers; // cycle through 0,1,2,..., numUsers-1
    }

}

void scheduleMaxCQI(const std::vector<User>& users, std::vector<int>& rbOwner) {
    int numUsers = static_cast<int>(users.size());

    for (int rb = 0; rb < static_cast<int>(rbOwner.size()); ++rb) {

        double bestCQI = -1.0;
        int bestUser = 0;

        for (int i = 0; i < numUsers; ++i) {
            if (users[i].cqi > bestCQI) {
                bestCQI = users[i].cqi;
                bestUser = i;
            }
        };

        rbOwner[rb] = bestUser;
    };
}

void schedulePF(const std::vector<User>& users, std::vector<int>& rbOwner,
    const std::vector<double>& bitsPerRbPerUser,
    const std::vector<double>& avgThroughput) {

    int numUsers = static_cast<int>(users.size());

    std::vector<double> tempAvg = avgThroughput;
    double pfAlpha = 0.01;

    for (int rb = 0; rb < (int)rbOwner.size(); rb++) {

        double bestMetric = -1.0;
        int bestUser = -1;

        for (int i = 0; i < numUsers; i++) {

            // Skip empty buffers
            if (users[i].buffer <= 0) continue;

            double instRate = bitsPerRbPerUser[i];
            double metric = instRate / tempAvg[i];

            if (metric > bestMetric) {
                bestMetric = metric;
                bestUser = i;
            }
        }

        if (bestUser == -1) {
            rbOwner[rb] = 0;  // nobody has buffer
            continue;
        }

        rbOwner[rb] = bestUser;

        // Update tempAvg to reduce metric for users already served
        double instRateBest = bitsPerRbPerUser[bestUser];
        tempAvg[bestUser] = (1.0 - pfAlpha) * tempAvg[bestUser]
            + pfAlpha * instRateBest;
    }
}
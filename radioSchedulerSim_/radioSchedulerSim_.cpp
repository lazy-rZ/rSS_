#include <iostream> 
#include <cstdlib> 
#include <ctime>
#include <vector>
#include <algorithm>
#include <cmath>

struct User {
    int id;
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

struct McsEntry {
    double cqiThreshold;
    double bitsPerRb;
};

double generateRandomCQI()
{
    return 15.0 * std::rand() / double(RAND_MAX); // generate number between 0 - 15
}

void updateCQI(User& u)
{
    u.cqi = generateRandomCQI();
}

double getBitsPerRbFromCQI(double cqi, const std::vector<McsEntry>& mcsTable) {
    // iterate from highest MCS to lowest
    for (int i = mcsTable.size() - 1; i >= 0; --i) {
        if (cqi >= mcsTable[i].cqiThreshold) { return mcsTable[i].bitsPerRb; }
    }

    // fallback
    return mcsTable[0].bitsPerRb;
}

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

double computeBaseBLER(double cqi) {
    // BLER decreases with CQI, better channel -> lower BLER
    return std::exp(-0.35 * cqi);
}

bool decideHarqACK(double cqi, int txCount) {
    double base = computeBaseBLER(cqi);
    // BLER decreases with number of transmissions
    double eff = std::pow(base, txCount);
    // For now draw the outcome using uniform dist.
    double u = std::rand() / double(RAND_MAX);
    
    return (u >= eff); // true = ACK otherwise NACK
}

int runHarqForUser(User& user, int capBits, double userCQI) {
    // No transmission at all this TTI
    if (capBits <= 0) {
        // If HARQ is active, TB just waits; if not, idle
        user.harqStatus = user.harqActive ? 'N' : '.';
        return 0;
    }
    
    if (user.harqActive) {
        int oldTb = user.harqBits;
        bool ack = decideHarqACK(userCQI, user.harqTxCount++);

        if (ack) {
            user.harqStatus = 'A';
            user.buffer -= oldTb;
            user.harqBits = 0;
            user.harqActive = false;
            return oldTb;
        }
        else {
            user.harqStatus = 'N';
            return 0;
        }
    }
    else {
        if (user.buffer <= 0 || capBits <= 0) {
            user.harqStatus = '.';
            return 0;
        }

        int tb = std::min(user.buffer, capBits);
        user.harqActive = true;
        user.harqBits = tb;
        user.harqTxCount = 1;

        bool ack = decideHarqACK(userCQI, 1);

        if (ack) {
            user.harqStatus = 'A';
            user.buffer -= tb;
            user.harqBits = 0;
            user.harqActive = false;
            return tb;
        }
        else {
            user.harqStatus = 'N';
            return 0;
        }
    }
}

int main()
{
    std::cout << "rSS_ | Build 0.5\n";
    
    // Initilize users
    std::vector<User> users;

    users.push_back({ 0, 0.0, 10000, 0.0, 0.0 });
    users.push_back({ 1, 0.0, 18000, 0.0, 0.0 });
    users.push_back({ 2, 0.0, 9000, 0.0, 0.0 });

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
            updateCQI(u); // update each users CQI using a random number generator
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


#include <iostream> 
#include <cstdlib> 
#include <ctime>
#include <vector>

struct User {
    int id;
    double cqi;
    int buffer;
    double bitsPerRb;
    double thr;
};

struct McsEntry {
    double cqiThreshold;
    double bitsPerRb;
};

double generateRandomCQI()
{
    return 15.0 * std::rand() / RAND_MAX; // generate number between 0 - 15
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

int main()
{
    std::cout << "rSS_ | Build 0.1\n";
    
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

    // Set seed to current time for rand
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    

    // main loop
    for (int tti = 0; tti < 10; ++tti) {
        
        std::cout << "TTI: " << tti << "\n";

        for (auto& u : users) { 
            updateCQI(u); // update each users CQI using a random number generator
        };

        // call scheduler
        scheduleRoundRobin(users, rbOwner);

        // Precompute bitsPerRb for each UE in this TTI
        for (std::size_t i = 0; i < users.size(); ++i) {
            bitsPerRbPerUser[i] = getBitsPerRbFromCQI(users[i].cqi, mcsTable);
        };

        // reset per UE throughput 
        std::vector<double> thrPerUser(users.size(), 0.0);

        // Accumulate throughput per UE by walking RBs
        for (int rb = 0; rb < TOTAL_RB; ++rb) {
            int uIndex = rbOwner[rb];
            thrPerUser[uIndex] += bitsPerRbPerUser[uIndex];
        };

        // Update buffers
        for (std::size_t i = 0; i < users.size(); ++i) {
            int bits = static_cast<int>(thrPerUser[i]);
            users[i].buffer = std::max(0, users[i].buffer - bits);
        };

        // clean out print
        for (std::size_t i = 0; i < users.size(); ++i) {
            const auto& u = users[i];
            std::cout << "  UE" << u.id
                << "  CQI=" << u.cqi
                << "  bitsPerRB=" << bitsPerRbPerUser[i]
                << "  THR=" << thrPerUser[i]
                << "  BUF=" << u.buffer << "\n";
        }
        
    }

    return 0;
}


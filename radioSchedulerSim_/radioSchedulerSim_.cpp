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

int main()
{
    std::cout << "rSS_ | Build 0.1\n";
    
    std::vector<User> users;

    users.push_back({ 0, 0.0, 10000, 0.0, 0.0 });
    users.push_back({ 1, 0.0, 18000, 0.0, 0.0 });
    users.push_back({ 2, 0.0, 9000, 0.0, 0.0 });

    std::vector<McsEntry> mcsTable = {
        { 0.0,  50  },   // QPSK low
        { 4.0,  80  },   // QPSK high
        { 7.0,  150 },   // 16QAM
        {10.0,  250 },   // 64QAM
        {13.0,  350 }    // 256QAM
    };

    int TOTAL_RB = 50;
    int rbPerUser = TOTAL_RB / users.size();

    std::srand(static_cast<unsigned>(std::time(nullptr))); // set seed to current time
    
    for (int tti = 0; tti < 10; ++tti) {
        
        std::cout << "TTI: " << tti << "\n";

        for (auto& u : users) { 
            updateCQI(u); // update each users CQI using a random number generator
            
            // get bitsPerRb and compute throughput
            double bitsPerRb = getBitsPerRbFromCQI(u.cqi, mcsTable);
            double thr = rbPerUser * bitsPerRb;
            u.buffer = std::max(0, u.buffer - (int)thr);

            u.thr = thr;
            u.bitsPerRb = bitsPerRb;
        };

        // clean out print
        for (const auto& u : users) {
            std::cout << "  UE" << u.id
                << "  CQI=" << u.cqi
                << "  bitsPerRB=" << u.bitsPerRb
                << "  THR=" << u.thr
                << "  RB=" << rbPerUser
                << "  BUF=" << u.buffer << "\n";
        }
        
    }

    return 0;
}


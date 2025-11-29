#include <cstdlib>
#include "user.h"
#include <vector>
#include <cmath>
#include "mcs.h"


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
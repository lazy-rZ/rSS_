#include <cstdlib>
#include "user.h"
#include <vector>
#include <cmath>
#include "mcs.h"
#include <algorithm>  

// Base station 
static const double BS_X = 0.0;
static const double BS_Y = 0.0;
static const double Ptx_dB = 46.0;

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

double computeDistance(const User& u) {
    double dx = u.x - BS_X;
    double dy = u.y - BS_Y;
    return std::sqrt(dx * dx + dy * dy);
}

double computePathloss_dB(double d) {
    // simple pathloss model defined as:
    // PL(d) = PL0 + 10 * n * log_10(d) 
    // where PL_0 = loss at 1 m, for example 30dB
    // n = pathloss exponent, we pick 3 which represent urban space
    return 30.0 + 30.0 * std::log10(d);
}

double computeSINR_dB(const User& u) {
    // get distance from BS to user -> compute loss -> compute recived power 
    // -> (thermal noise) -> compute SINR
    double distance = computeDistance(u);
    double pathLoss_dB = computePathloss_dB(distance);
    double Prx_dB = Ptx_dB - pathLoss_dB;
    double noise_dB = -101.0;
    return Prx_dB - noise_dB;
}

double sinrToCQI(double sinr_dB) {
    // simple linear map
    double cqi = (sinr_dB - 30.0) / 4.0; // change sensitivity 
    return std::clamp(cqi, 0.0, 15.0);
}

void updateCQI_physical(User& u) {
    double sinr_dB = computeSINR_dB(u);
    u.cqi = sinrToCQI(sinr_dB);
}
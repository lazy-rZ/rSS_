#pragma once
#include "mcs.h"
#include <vector>

double generateRandomCQI();

void updateCQI(User&);

double computeBaseBLER(double CQI);

bool decideHarqACK(double CQI, int txCount);

double getBitsPerRbFromCQI(double cqi, const std::vector<McsEntry>& table);

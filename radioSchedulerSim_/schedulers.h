#pragma once
#include <vector>
#include "user.h"

void schedulePF(const std::vector<User>& users,
    std::vector<int>& rbOwner,
    const std::vector<double>& bitsPerRbPerUser,
    const std::vector<double>& avgThroughput);

void scheduleMaxCQI(const std::vector<User>& users, std::vector<int>& rbOwner);

void scheduleRoundRobin(const std::vector<User>& users, std::vector<int>& rbOwner);
#include "user.h"
#include "channel.h"
#include <algorithm>

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
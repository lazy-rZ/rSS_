#include <iostream> 
#include <cstdlib> 
#include <ctime>
#include <vector>

struct User {
    int id;
    double cqi;
    int buffer;
};

double generateRandomCQI()
{
    return 15.0 * std::rand() / RAND_MAX; // generate number between 0 - 15
}

void updateCQI(User& u)
{
    u.cqi = generateRandomCQI();
}

int main()
{
    std::cout << "rSS_ | Build 0.1\n";
    
    std::vector<User> users;

    users.push_back({ 0, 0.0, 10000 });
    users.push_back({ 1, 0.0, 18000 });
    users.push_back({ 2, 0.0, 9000 });

    std::srand(static_cast<unsigned>(std::time(nullptr))); // set seed to current time
    
    for (int tti = 0; tti < 10; ++tti) {
        
        for (auto& u : users) { updateCQI(u); };
        
        std::cout << "TTI: " << tti << "\n";

        for (auto& u : users)
        {
            std::cout << "  UE: " << u.id
                << ": CQI= " << u.cqi
                << "  BUF= " << u.buffer << "\n";
        }; 
        
    }

    return 0;
}


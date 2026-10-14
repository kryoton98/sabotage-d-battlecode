#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace queen_safety {
struct Board {
    int size=0, goal=-1;
    bool portal_last=false, retreat=false;
    std::vector<std::array<int,4>> next;
    std::vector<std::array<uint8_t,4>> portal;
    std::vector<uint8_t> known, visible, blocked;
    std::vector<int> food_index, body, enemies;
    uint64_t food=0;
};
struct Walk {
    std::vector<int> body;
    uint64_t food=0;
    std::string path;
    int eaten=0, paid=0;
    bool portal=false;
};
struct Decision {
    std::string path;
    bool changed=false, searched=false, limited=false, emergency=false;
    int nodes=0, clearance_before=0, clearance_after=0;
};
uint64_t ticks();
bool step(const Board&,Walk&,int direction,bool paid,bool portals);
bool uses_portal(const Board&,const std::string&);
Decision choose(const Board&,const std::string& incumbent,uint64_t epoch);
}

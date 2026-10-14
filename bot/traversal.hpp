#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace traversal {
// The search changes ordinary worker travel only. All occupied squares belong
// to the current observation; terrain distance may use older map memory.
struct Board {
    int size=0, goal=-1;
    std::vector<std::array<int,4>> next;
    std::vector<uint8_t> known, visible, blocked, interior;
    std::vector<int> food_index;
    uint64_t food=0;
    std::vector<int> body;
};
struct Walk {
    std::array<int16_t,144> body{};
    int length=0, eaten=0;
    uint64_t food=0;
    std::string path;
};
struct Budget {
    uint64_t deadline=0;
    int nodes=0, cap=9000;
    bool limited=false;
    bool check();
};
struct Decision {
    std::string path;
    bool searched=false, changed=false, limited=false;
    int nodes=0, extra_food=0, progress=0;
};
uint64_t ticks();
bool step(const Board&,Walk&,int direction);
bool continuation(const Board&,const Walk&,int depth,Budget&);
Decision choose(const Board&,const std::string& incumbent,uint64_t epoch);
}

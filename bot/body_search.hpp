#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace body_search {
uint64_t ticks();
struct Budget {
    uint64_t epoch=0;
    int nodes=0;
    bool limited=false;
    bool check();
};
struct Result {bool complete=false;int depth=0;};
// Only a complete answer may replace the existing survival estimate. Unknown
// terrain is an unresolved escape, as in the existing exploration policy.
Result search(const std::vector<std::array<int,4>>& next,
              const std::vector<uint8_t>& known,
              const std::vector<uint8_t>& food,
              const std::vector<uint8_t>& blocked,
              const std::vector<int>& body,int start,int grow,int need,
              bool require_known,Budget&);
}

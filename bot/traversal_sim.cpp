#include "traversal.hpp"
#include <algorithm>
#include <ctime>

namespace traversal {
uint64_t ticks() {
    timespec t{};
    clock_gettime(CLOCK_MONOTONIC,&t);
    return uint64_t(t.tv_sec)*1000000000ULL+uint64_t(t.tv_nsec);
}
bool Budget::check() {
    if(limited) return false;
    if(++nodes>=cap || ticks()>=deadline) limited=true;
    return !limited;
}
bool step(const Board& b,Walk& w,int direction) {
    if(direction<0 || direction>3 || w.length<2 || w.length>=143) return false;
    int from=w.body[0], to=b.next[from][direction];
    if(to<0 || to>=b.size || !b.visible[to] || !b.known[from] || b.blocked[to]) return false;
    // Collision is checked BEFORE the tail leaves, including on the last tail.
    for(int k=0;k<w.length;++k) if(w.body[k]==to) return false;
    int bit=b.food_index[to];
    bool eat=bit>=0 && (w.food&(1ULL<<bit));
    if(eat) {w.food&=~(1ULL<<bit);++w.eaten;}
    int length=w.length+int(eat);
    for(int k=length-1;k>0;--k) w.body[k]=w.body[k-1];
    w.body[0]=int16_t(to);w.length=length;
    return true;
}
bool continuation(const Board& b,const Walk& start,int depth,Budget& budget) {
    if(depth==0) return true;
    if(!budget.check()) return false;
    // This is an existence check with exact moving bodies, not a prediction of
    // future enemy or teammate behaviour. Other observed bodies stay blocked.
    for(int d=0;d<4;++d) {
        Walk next=start;
        if(step(b,next,d) && continuation(b,next,depth-1,budget)) return true;
        if(budget.limited) return false;
    }
    return false;
}
}

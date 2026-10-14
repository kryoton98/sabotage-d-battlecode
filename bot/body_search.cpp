#include "body_search.hpp"
#include <algorithm>
#include <ctime>

namespace body_search {
uint64_t ticks() {
    timespec ts{};clock_gettime(CLOCK_MONOTONIC,&ts);
    return uint64_t(ts.tv_sec)*1000000000ULL+uint64_t(ts.tv_nsec);
}
bool Budget::check() {
    if(limited) return false;
    if(++nodes>=14000 || ticks()>=epoch+52000000ULL) limited=true;
    return !limited;
}
namespace {
struct Search {
    const std::vector<std::array<int,4>>& next;
    const std::vector<uint8_t>& known;
    const std::vector<uint8_t>& food;
    const std::vector<uint8_t>& blocked;
    Budget& budget;
    std::array<int,256> ring{};
    std::vector<uint8_t> occupied,eaten;
    int front=0,length=0,need=0,best=0;
    bool require_known=false;

    bool walk(int g,int depth,int force_growth=-1) {
        if(!budget.check()) return false;
        if(g<0 || g>=int(next.size()) || occupied[g] || blocked[g]) return false;
        // No claimed trap can depend on unobserved walls. Conservative queen
        // searches may explicitly require remembered terrain throughout.
        if(!known[g]) {
            if(require_known) return false;
            best=need;return true;
        }
        bool eat=force_growth>=0?force_growth!=0:(food[g]&&!eaten[g]);
        if(length+int(eat)>=256) {budget.limited=true;return false;}
        int old_front=front,old_length=length;
        int tail=ring[(front+length-1)&255];
        int new_front=(front+255)&255,previous_slot=ring[new_front];
        bool old_eaten=eaten[g];
        if(!eat) occupied[tail]=0;
        front=new_front;ring[front]=g;length+=int(eat);occupied[g]=1;
        if(eat) eaten[g]=1;
        best=std::max(best,depth);
        bool done=depth>=need;
        if(!done) for(int to:next[g]) {
            if(walk(to,depth+1)) {done=true;break;}
            if(budget.limited) break;
        }
        occupied[g]=0;if(!eat) occupied[tail]=1;
        eaten[g]=old_eaten;ring[new_front]=previous_slot;front=old_front;length=old_length;
        return done;
    }
};
}
Result search(const std::vector<std::array<int,4>>& next,const std::vector<uint8_t>& known,
              const std::vector<uint8_t>& food,const std::vector<uint8_t>& blocked,
              const std::vector<int>& body,int start,int grow,int need,bool require_known,Budget& budget) {
    Result r;
    int n=int(next.size());
    if(n<1 || n>4096 || body.size()<2 || body.size()>128 || need<2 || need>96 || grow<0 || grow>1 ||
       int(known.size())!=n || int(food.size())!=n || int(blocked.size())!=n || !budget.check()) return r;
    if(start<0 || start>=n || std::find(next[body[0]].begin(),next[body[0]].end(),start)==next[body[0]].end()) return r;
    Search s{next,known,food,blocked,budget};s.occupied.assign(n,0);s.eaten.assign(n,0);
    s.length=int(body.size());s.need=need;s.require_known=require_known;
    for(int k=0;k<s.length;++k) {
        int g=body[k];if(g<0 || g>=n || s.occupied[g]) return r;
        s.ring[k]=g;s.occupied[g]=1;
    }
    // Calls representing a future starting state are outside this adapter.
    if(s.occupied[start] || blocked[start]) return r;
    s.walk(start,1,grow);
    r.complete=!budget.limited;r.depth=s.best;
    return r;
}
}

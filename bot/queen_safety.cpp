#include "queen_safety.hpp"
#include <algorithm>
#include <ctime>
#include <tuple>

namespace queen_safety {
namespace {
constexpr char dirs[]="NESW";
struct Budget {
    uint64_t deadline;
    int nodes=0;
    bool limited=false;
    bool check() {
        if(limited) return false;
        if(++nodes>6000 || ticks()>=deadline) limited=true;
        return !limited;
    }
};
int direction(char c) {return int(std::find(dirs,dirs+4,c)-dirs);}
}
uint64_t ticks() {
    timespec ts{};clock_gettime(CLOCK_MONOTONIC,&ts);
    return uint64_t(ts.tv_sec)*1000000000ULL+uint64_t(ts.tv_nsec);
}
bool step(const Board& b,Walk& w,int d,bool paid,bool portals) {
    if(d<0 || d>3 || w.body.size()<2 || (paid && w.body.size()<3)) return false;
    int from=w.body[0],to=b.next[from][d];
    if(b.portal[from][d] && !portals) return false;
    if(to<0 || to>=b.size || !b.visible[to] || b.blocked[to] || !b.known[from]) return false;
    // The tail is still occupied during the collision check.
    if(std::find(w.body.begin(),w.body.end(),to)!=w.body.end()) return false;
    int bit=b.food_index[to];bool eat=bit>=0 && (w.food&(1ULL<<bit));
    if(eat) {w.food&=~(1ULL<<bit);++w.eaten;}
    if(paid) ++w.paid;
    w.portal=w.portal || b.portal[from][d];
    w.body.insert(w.body.begin(),to);
    if(!eat) w.body.pop_back();
    if(paid) w.body.pop_back();
    return true;
}
bool uses_portal(const Board& b,const std::string& path) {
    if(b.body.empty()) return false;
    int g=b.body[0];
    for(char c:path) {
        int d=direction(c);if(d>3 || g<0 || g>=b.size) return false;
        if(b.portal[g][d]) return true;
        g=b.next[g][d];
    }
    return false;
}
namespace {
std::vector<int> enemy_distance(const Board& b,Budget& budget) {
    std::vector<int> dist(b.size,10000),queue;
    for(int g:b.enemies) if(g>=0 && g<b.size && dist[g]) {dist[g]=0;queue.push_back(g);}
    for(size_t at=0;at<queue.size();++at) {
        if((at&31)==0 && !budget.check()) break;
        int g=queue[at];
        if(!b.known[g] || dist[g]>=12) continue;
        // Terrain distance includes enemy use of known portals. Bodies may move
        // before an enemy acts, so they must not make an enemy seem farther away.
        for(int to:b.next[g]) if(to>=0 && to<b.size && dist[to]>dist[g]+1) {
            dist[to]=dist[g]+1;queue.push_back(to);
        }
    }
    return dist;
}
int exits(const Board& b,const Walk& w,bool portals) {
    int n=0;
    for(int d=0;d<4;++d) {Walk next=w;if(step(b,next,d,false,portals)) ++n;}
    return n;
}
bool continuation(const Board& b,const Walk& w,int depth,bool portals,Budget& budget) {
    if(!depth) return true;
    if(!budget.check()) return false;
    for(int d=0;d<4;++d) {
        Walk next=w;
        if(step(b,next,d,false,portals) && continuation(b,next,depth-1,portals,budget)) return true;
        if(budget.limited) return false;
    }
    return false;
}
}
Decision choose(const Board& b,const std::string& incumbent,uint64_t epoch) {
    Decision out;out.path=incumbent;
    if(b.size<1 || b.size>4096 || b.body.size()<2 || b.body.size()>size_t(b.size)) return out;
    bool portal_action=b.portal_last && uses_portal(b,incumbent);
    if(!b.retreat && !portal_action) return out;
    out.searched=true;
    Budget budget{epoch+62000000ULL};
    auto dist=enemy_distance(b,budget);
    out.clearance_before=std::min(12,dist[b.body[0]]);
    const int free_steps=(int(b.body.size())+3)/4;
    Walk initial;initial.body=b.body;initial.food=b.food;
    auto rank=[&](const Walk& w) {
        int clearance=b.retreat?std::min(12,dist[w.body[0]]):0;
        return std::make_tuple(clearance,int(w.body.size()),-w.paid,-int(w.path.size()));
    };
    auto select=[&](bool portals,Walk& answer,int& quality) {
        // Always examine all four first steps, even if the deeper search has
        // no budget left. This keeps the portal rule effective at the cutoff.
        std::vector<Walk> beam, finalists;
        for(int d=0;d<4;++d) {
            Walk w=initial;
            if(step(b,w,d,false,portals)) {w.path=std::string(1,dirs[d]);beam.push_back(w);finalists.push_back(std::move(w));}
        }
        Walk base=initial;bool valid=!incumbent.empty();
        for(size_t k=0;k<incumbent.size() && valid;++k)
            valid=step(b,base,direction(incumbent[k]),int(k)>=free_steps,portals);
        if(valid) {base.path=incumbent;finalists.push_back(base);}
        int depth_cap=std::min(8,free_steps+(b.retreat?3:0));
        for(int depth=1;depth<depth_cap && !beam.empty() && !budget.limited;++depth) {
            std::vector<Walk> next;
            for(const auto& w:beam) {
                for(int d=0;d<4;++d) {
                    if(!budget.check()) break;
                    Walk v=w;
                    if(!step(b,v,d,depth>=free_steps,portals)) continue;
                    v.path+=dirs[d];next.push_back(std::move(v));
                }
                if(budget.limited) break;
            }
            std::stable_sort(next.begin(),next.end(),[&](const Walk& a,const Walk& c){return rank(a)>rank(c);});
            if(next.size()>12) next.resize(12);
            for(const auto& w:next) finalists.push_back(w);
            beam=std::move(next);
        }
        std::stable_sort(finalists.begin(),finalists.end(),[&](const Walk& a,const Walk& c){
            if(rank(a)!=rank(c)) return rank(a)>rank(c);
            return a.path==incumbent && c.path!=incumbent;
        });
        if(finalists.empty()) return false;
        answer=finalists[0];quality=-1;
        // A complete failed continuation distinguishes a known dead end from
        // a search that merely ran out of time. It cannot predict moving foes.
        for(const auto& w:finalists) {
            int grade=0;
            if(budget.limited) grade=1;
            else if(continuation(b,w,std::min(16,std::max(6,int(w.body.size())+2)),portals,budget)) grade=2;
            else if(budget.limited) grade=1;
            if(grade>quality) {answer=w;quality=grade;}
            if(grade==2) break;
        }
        return true;
    };
    Walk normal;int quality=-1;
    bool have=select(!b.portal_last,normal,quality);
    if(have && (quality>0 || !b.portal_last)) {
        out.path=normal.path;out.changed=out.path!=incumbent;
        out.clearance_after=std::min(12,dist[normal.body[0]]);
    } else {
        Walk escape;int escape_quality=-1;
        bool portal_exit=b.portal_last && select(true,escape,escape_quality) && escape.portal &&
                         (!have || escape_quality>quality);
        if(portal_exit) {
            out.path=escape.path;out.changed=out.path!=incumbent;out.emergency=true;
            out.clearance_after=std::min(12,dist[escape.body[0]]);
        } else {
            // Unknown portal destinations are a last chance only. Do not
            // pretend their unseen landing square has been checked for safety.
            int unknown=-1;
            for(int d=0;d<4;++d) if(b.portal[b.body[0]][d]) {
                int to=b.next[b.body[0]][d];
                if(to==-3 || (to>=0 && !b.visible[to])) {unknown=d;break;}
            }
            if(unknown>=0 && (!have || (quality==0 && !budget.limited))) {
                out.path=std::string(1,dirs[unknown]);out.changed=out.path!=incumbent;out.emergency=true;
            } else if(have) {
                out.path=normal.path;out.changed=out.path!=incumbent;
                out.clearance_after=std::min(12,dist[normal.body[0]]);
            }
        }
    }
    out.nodes=budget.nodes;out.limited=budget.limited;
    return out;
}
}

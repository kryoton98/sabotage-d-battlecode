#include "traversal.hpp"
#include <algorithm>
#include <tuple>

namespace traversal {
static constexpr char dirs[]="NESW";
static std::vector<int> distances(const Board& b,Budget& budget) {
    std::vector<int> dist(b.size,100000);
    if(b.goal<0 || b.goal>=b.size || !b.known[b.goal]) return dist;
    std::vector<std::vector<int>> back(b.size);
    for(int g=0;g<b.size;++g) {
        if((g&31)==0 && !budget.check()) return dist;
        if(!b.known[g]) continue;
        for(int to:b.next[g]) if(to>=0 && to<b.size && b.known[to]) back[to].push_back(g);
    }
    std::vector<int> q{b.goal};dist[b.goal]=0;
    for(size_t i=0;i<q.size();++i) {
        if((i&31)==0 && !budget.check()) return dist;
        int g=q[i];
        for(int from:back[g]) if(dist[from]>dist[g]+1) {dist[from]=dist[g]+1;q.push_back(from);}
    }
    return dist;
}
static int exits(const Board& b,const Walk& w) {
    int result=0;
    for(int d=0;d<4;++d) {Walk v=w;if(step(b,v,d)) ++result;}
    return result;
}
Decision choose(const Board& b,const std::string& incumbent,uint64_t epoch) {
    Decision out;out.path=incumbent;
    if(b.size<1 || b.size>4096 || b.body.size()<5 || b.body.size()>128 || incumbent.empty()) return out;
    const int free_steps=(int(b.body.size())+3)/4;
    if(int(incumbent.size())>free_steps || incumbent.size()>12) return out;
    if(!epoch) epoch=ticks();
    Budget budget;budget.deadline=epoch+62000000ULL;
    if(!budget.check()) {out.limited=true;return out;}
    Walk initial;initial.length=int(b.body.size());initial.food=b.food;
    for(int k=0;k<initial.length;++k) initial.body[k]=int16_t(b.body[k]);
    Walk base=initial;
    for(char c:incumbent) {
        const char* p=std::find(dirs,dirs+4,c);
        if(p==dirs+4 || !step(b,base,int(p-dirs))) return out;
    }
    base.path=incumbent;
    Walk first=initial;
    int first_direction=int(std::find(dirs,dirs+4,incumbent[0])-dirs);
    if(!step(b,first,first_direction)) return out;
    first.path=incumbent.substr(0,1);
    out.searched=true;
    auto dist=distances(b,budget);
    int base_dist=dist[base.body[0]];
    auto rank=[&](const Walk& w) {
        // Immediate food first; among equal harvests follow the same destination.
        return std::make_tuple(w.eaten,-dist[w.body[0]],-int(w.path.size()));
    };
    auto better=[&](const Walk& w) {
        if(w.path==incumbent || !b.interior[w.body[0]]) return false;
        if(w.eaten<base.eaten || dist[w.body[0]]>base_dist) return false;
        return w.eaten>base.eaten || dist[w.body[0]]<base_dist;
    };
    std::vector<Walk> beam{first}, finalists;
    for(int depth=1;depth<=std::min(12,free_steps) && !budget.limited;++depth) {
        for(const auto& w:beam) if(better(w)) finalists.push_back(w);
        std::sort(finalists.begin(),finalists.end(),[&](const Walk& a,const Walk& c){return rank(a)>rank(c);});
        if(finalists.size()>24) finalists.resize(24);
        if(depth==std::min(12,free_steps)) break;
        std::vector<Walk> next;
        for(const auto& w:beam) {
            for(int d=0;d<4;++d) {
                if(!budget.check()) break;
                Walk v=w;
                if(!step(b,v,d)) continue;
                v.path+=dirs[d];next.push_back(std::move(v));
            }
            if(budget.limited) break;
        }
        std::sort(next.begin(),next.end(),[&](const Walk& a,const Walk& c){return rank(a)>rank(c);});
        if(next.size()>32) next.resize(32);
        beam=std::move(next);
    }
    const int need_exits=std::min(2,exits(b,base));
    for(const auto& w:finalists) {
        if(!budget.check()) break;
        if(exits(b,w)<std::max(1,need_exits)) continue;
        int horizon=std::min(24,std::max(12,w.length+4));
        if(!continuation(b,w,horizon,budget)) continue;
        out.path=w.path;out.changed=true;out.extra_food=w.eaten-base.eaten;
        out.progress=base_dist-dist[w.body[0]];
        break;
    }
    out.nodes=budget.nodes;out.limited=budget.limited;
    return out;
}
}

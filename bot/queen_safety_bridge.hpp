// Final queen-only policy, after the normal strategy has proposed its action.
std::string protect_queen(const std::vector<std::string>& L,const std::string& original) {
    if(!is_queen || (!T(P.QUEEN_PORTAL_LAST) && !T(P.QUEEN_RETREAT))) return original;
    queen_safety::Board b;b.size=N;b.goal=food_goal;
    b.portal_last=T(P.QUEEN_PORTAL_LAST) && N>=1100;
    b.next=nbrs;b.portal.resize(N);b.known.resize(N);b.visible.assign(N,0);
    b.blocked.assign(N,0);b.food_index.assign(N,-1);
    for(int g=0;g<N;++g) {
        b.known[g]=seen[g]!=UNSEEN;
        const PTile* p=ptile(g);
        for(int d=0;d<4;++d) b.portal[g][d]=p && p->get(d,-1)>=0;
    }
    int pos=5+int(pyint(tail_from(at(L,4),9)));
    if(starts_with(at(L,pos),"ECHOES")) ++pos;
    for(int k=0;k<49;++k) {
        auto w=split_ws(at(L,pos++));int g=int(pyint(w[1]))*W+int(pyint(w[0]));
        b.known[g]=b.visible[g]=1;b.food_index[g]=k;
        if(pyint(w[2])) b.food|=1ULL<<k;
    }
    int count=int(pyint(tail_from(at(L,pos++),14)));
    std::map<int,std::vector<int>> enemy_parts;
    std::map<int,int> enemy_heads;
    std::vector<int> own_seen;
    for(int k=0;k<count;++k) {
        auto w=split_ws(at(L,pos++));int uid=int(pyint(w[1]));
        int g=int(pyint(w[3]))*W+int(pyint(w[2]));
        if(uid==id) {own_seen.push_back(g);continue;}
        b.blocked[g]=1;
        if((w[0]=="B"?1:0)!=tbit) {
            enemy_parts[uid].push_back(g);
            if(w[5]=="1") enemy_heads[uid]=g;
        }
    }
    b.retreat=T(P.QUEEN_RETREAT) && !enemy_parts.empty();
    if(!b.retreat && !b.portal_last) return original;
    for(const auto& p:enemy_parts) {
        auto it=enemy_heads.find(p.first);
        if(it!=enemy_heads.end()) b.enemies.push_back(it->second);
        else b.enemies.insert(b.enemies.end(),p.second.begin(),p.second.end());
    }
    b.body=trail;
    if(b.body.empty() || b.body[0]!=my_hg || int(b.body.size())!=my_len) {
        // On an incomplete trail use observed own segments as fixed obstacles.
        // A two-cell placeholder cannot grant any extra free movement.
        for(int g:own_seen) b.blocked[g]=1;
        b.body={my_hg,my_hg};
    }
    size_t start=original.find("MOVE ");
    size_t split_start=original.find("SPLIT ");
    size_t action=start!=std::string::npos?start:split_start;
    if(action==std::string::npos) return original;
    size_t end=original.find('\n',action);
    std::string incumbent=start==std::string::npos?std::string():
        original.substr(action+5,end==std::string::npos?end:end-action-5);
    auto choice=queen_safety::choose(b,incumbent,traversal_epoch);
    if(!choice.searched) return original;
    std::string mark="INDICATOR queen-policy ";
    if(b.retreat) mark+="queen-retreat ";
    if(b.portal_last) mark+="queen-portals-last ";
    if(choice.emergency) mark+="queen-portal-emergency ";
    if(choice.limited) mark+="queen-policy-limit ";
    mark+=choice.changed?"queen-policy-change\n":"queen-policy-keep\n";
    if(!choice.changed || choice.path.empty()) return mark+original;
    route_dirty=true;has_plan=false;plan.clear();spr_r=-10;
    std::vector<int> path;for(char c:choice.path) path.push_back(didx(c));
    return mark+original.substr(0,action)+emit_move(path,my_hg,"")+
           (end==std::string::npos?"":original.substr(end));
}

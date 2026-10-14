// Ordinary worker movement may search different FREE paths after the same first
// step. The turn code explicitly opts in; other strategic actions never opt in.
uint64_t traversal_epoch=0;
bool traversal_allowed=false;
std::string improve_travel(const std::vector<std::string>& L,const std::string& baseline) {
    if(!T(P.TRAVEL_SEARCH) || !traversal_allowed || is_queen || my_len<5 || my_len>128 ||
       int(trail.size())!=my_len || trail.empty() || trail[0]!=my_hg) return baseline;
    if(traversal::ticks()>=traversal_epoch+62000000ULL) return baseline;
    std::string path;
    size_t action_start=baseline.find("MOVE ");
    if(action_start==std::string::npos) return baseline;
    size_t end=baseline.find('\n',action_start);
    path=baseline.substr(action_start+5,end==std::string::npos?end:end-action_start-5);
    traversal::Board b;b.size=N;b.next=nbrs;
    b.goal=food_goal>=0?food_goal:(has_route?route_goal:-1);b.body=trail;
    b.known.resize(N);b.visible.assign(N,0);b.blocked.assign(N,0);b.interior.assign(N,0);b.food_index.assign(N,-1);
    for(int g=0;g<N;++g) b.known[g]=seen[g]!=UNSEEN;
    int pos=5+int(pyint(tail_from(at(L,4),9)));
    if(starts_with(at(L,pos),"ECHOES")) ++pos;
    for(int k=0;k<49;++k) {
        auto w=split_ws(at(L,pos++));int g=int(pyint(w[1]))*W+int(pyint(w[0]));
        b.visible[g]=b.known[g]=1;b.food_index[g]=k;
        b.interior[g]=k/7>0 && k/7<6 && k%7>0 && k%7<6;
        if(pyint(w[2])) b.food|=1ULL<<k;
    }
    int n=int(pyint(tail_from(at(L,pos++),14)));
    for(int k=0;k<n;++k) {
        auto w=split_ws(at(L,pos++));int uid=int(pyint(w[1]));
        if(uid==id) continue;
        // Combat, the queen's immediate space and her food are left to the
        // existing roles, even if only one distant segment is visible.
        if((w[0]=="B"?1:0)!=tbit || uid<2) return baseline;
        int g=int(pyint(w[3]))*W+int(pyint(w[2]));b.blocked[g]=1;
    }
    auto choice=traversal::choose(b,path,traversal_epoch);
    std::string mark;
    if(T(P.TRAVEL_TRACE) && choice.searched) {
        mark="INDICATOR travel-search ";mark+=choice.changed?"travel-change ":"travel-keep ";
        if(choice.limited) mark+="travel-limit ";
        if(choice.changed && choice.extra_food>0) mark+="travel-food ";
        if(choice.changed && choice.progress>0) mark+="travel-progress ";
        mark+="\n";
    }
    if(!choice.changed) return mark+baseline;
    route_dirty=true;has_plan=false;plan.clear();spr_r=-10;
    std::vector<int> result;for(char c:choice.path) result.push_back(didx(c));
    std::string replacement=emit_move(result,my_hg,"");
    return mark+baseline.substr(0,action_start)+replacement+(end==std::string::npos?"":baseline.substr(end));
}

body_search::Budget body_budget;
bool body_prepared=false,body_checked=false,body_refined=false;
std::vector<uint8_t> body_known,body_food;
body_search::Result exact_body_survival(int start,int need,int grow,int length,const TileSet* oth) {
    body_search::Result none;
    if(!T(P.BODY_SEARCH) || length!=my_len || int(trail.size())!=length || trail.empty() || trail[0]!=my_hg) return none;
    if(!body_budget.check()) return none;
    if(!body_prepared) {
        body_known.resize(N);body_food.assign(N,0);
        for(int g=0;g<N;++g) body_known[g]=seen[g]!=UNSEEN;
        for(int g:pearls.items) if(g>=0 && g<N) body_food[g]=1;
        body_prepared=true;
    }
    std::vector<uint8_t> blocked(N,0);
    if(oth) for(int g:oth->items) if(g>=0 && g<N) blocked[g]=1;
    auto answer=body_search::search(nbrs,body_known,body_food,blocked,trail,start,grow,need,surv_seen,body_budget);
    body_checked=true;
    return answer;
}

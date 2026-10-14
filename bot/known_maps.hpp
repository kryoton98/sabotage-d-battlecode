// Sabotage-d / known_maps.hpp: (8 Oct, ORA) recognising a ladder map from what is in view (ora_check) and taking its
// walls, rates and parameters once it is recognised (ora_apply). The maps themselves are in known_maps_data.hpp.
// Member functions of struct Brain: included once from brain.hpp, inside the struct.

    // (8 Oct, ORA) is this one of the ladder maps? Every edge in view (kelp / portal / open) and every tile's timer (barren
    // or not; a countdown no longer than the map's longest) must match the candidate's. The kelp / portal edges seen, each
    // counted once, are the evidence: one candidate left with ORA_MIN of them = recognised. After that the same check runs
    // every turn; a mismatch undoes the map's parameters and rates (its walls, if given, stay known).
    void ora_check(const std::vector<std::string>& L, long long el0, const int* wg, const int* wcd, int rnd) {
        int obs_k[112], obs_g[112], obs_c[112];
        int no = 0;
        auto cls = [](const std::string& t) -> int { return t == "." ? 0 : (t == "w" ? 1 : 2); };
        for (int r = 0; r < 8; r++) {
            auto toks = split_ws(at(L, el0 + r));
            for (int c = 0; c < 7 && c < (int)toks.size(); c++) {
                obs_k[no] = 0;
                obs_g[no] = r < 7 ? wg[r * 7 + c] : plain_(wg[42 + c], 2);
                obs_c[no++] = cls(toks[(size_t)c]);
            }
        }
        for (int r = 0; r < 7; r++) {
            auto toks = split_ws(at(L, el0 + 8 + r));
            for (int c = 0; c < 8 && c < (int)toks.size(); c++) {
                obs_k[no] = 1;
                obs_g[no] = c < 7 ? wg[r * 7 + c] : plain_(wg[r * 7 + 6], 1);
                obs_c[no++] = cls(toks[(size_t)c]);
            }
        }
        std::vector<int> keep_c;
        std::vector<std::vector<uint8_t>> keep_k, keep_h;
        for (size_t ci = 0; ci < ora_cand.size(); ci++) {
            const auto& kd = ora_kinds[ci];
            const auto& hd = ora_his[ci];
            bool ok = (int)kd.size() == N && (int)hd.size() == N;
            for (int i = 0; i < no && ok; i++)
                if (((kd[(size_t)obs_g[i]] >> (2 * obs_k[i])) & 3) != obs_c[i]) ok = false;
            for (int i = 0; i < 49 && ok; i++) {
                const int hb = hd[(size_t)wg[i]];
                const int cd = wcd[i];
                if (hb == 0 ? cd >= 0 : (cd < 0 || (hb < 255 && cd > hb))) ok = false;
            }
            if (ok) {
                keep_c.push_back(ora_cand[ci]);
                keep_k.push_back(std::move(ora_kinds[ci]));
                keep_h.push_back(std::move(ora_his[ci]));
            }
        }
        ora_cand.swap(keep_c);
        ora_kinds.swap(keep_k);
        ora_his.swap(keep_h);
        if (ora_cand.empty()) {
            if (ora_state == 1) {
                // recognised before, not any more: back to my own parameters
                if (ora_P0) P = *ora_P0;
                pos_learn = ora_pos0;
                ora_rates_on = false;
                if (P.DEBUG >= 1) fprintf(stderr, "ORA %d id%d undone %s\n", rnd, id, KMAPS[ora_map].name);
            } else if (P.DEBUG >= 1) {
                fprintf(stderr, "ORA %d id%d unknown\n", rnd, id);
            }
            ora_state = -1;
            ora_kinds.clear();
            ora_his.clear();
            ora_counted.clear();
            return;
        }
        if (ora_state == 1) return;
        for (int i = 0; i < no; i++)
            if (obs_c[i] != 0) {
                const size_t key = (size_t)(obs_k[i] * N + obs_g[i]);
                if (!ora_counted[key]) {
                    ora_counted[key] = 1;
                    ora_info++;
                }
            }
        if (ora_cand.size() == 1 && ora_info >= P.ORA_MIN) {
            ora_state = 1;
            ora_map = ora_cand[0];
            ora_counted.clear();
            if (P.DEBUG >= 1) fprintf(stderr, "ORA %d id%d map %s\n", rnd, id, KMAPS[ora_map].name);
            ora_apply();
        }
    }

    void ora_apply() {   // (8 Oct, ORA) the map is known: its parameters, walls and spawn rates
        const KMap& km = KMAPS[ora_map];
        ora_P0 = P;
        ora_pos0 = pos_learn;
        for (auto& o : P.map_ovr)
            if (o.k == ora_map) P.*(o.field) = o.v;
        if (T(P.ORA_WALLS)) {
            const auto& kd = ora_kinds[0];
            for (int g = 0; g < N; g++) {
                if ((kd[(size_t)g] & 3) == 1) edge_(0, g, "w");
                if (((kd[(size_t)g] >> 2) & 3) == 1) edge_(1, g, "w");
            }
            for (int i = 0; i + 2 < 3 * km.np; i += 3) edge_(km.port[i], km.port[i + 1], std::to_string(km.port[i + 2]));
        }
        if (P.ORA_W > 0) {
            if (pos_rate.size() != (size_t)N) {
                pos_rate.assign((size_t)N, -1.0);
                pos_due.assign((size_t)N, INT_MIN);
                pos_r5.assign((size_t)N, 0.0);
                pos_n5.assign((size_t)N, 0);
            }
            pos_learn = true;
            ora_rates_on = true;
            P.POS_W = P.ORA_W;
            const auto rt = b64dec(km.rates);
            for (int g = 0; g < N && g < (int)rt.size(); g++) {
                const double q = rt[(size_t)g] / 255.0;
                pos_set(g, q * q);
            }
        }
    }

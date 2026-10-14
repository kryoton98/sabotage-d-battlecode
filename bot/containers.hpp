// Sabotage-d / containers.hpp: the containers that stand in for Python's sets and dicts (TileSet, IntMap,
// OrderedMap, SmallMap, PTile) and the small records the turn passes around (Tri, Head, Enemy, Scored, Cand).
// A piece of main.cpp: included once, inside its namespace. Not a header to include from anywhere else.

// ------------------------------------------------------------------------------------------------ containers
const int NOVAL = INT_MIN;

// a set of tiles (Python set of ints), cleared in O(size)
struct TileSet {
    std::vector<char> mark;
    std::vector<int> items;
    void init(int n) {
        mark.assign((size_t)n, 0);
        items.clear();
    }
    bool has(int g) const { return g >= 0 && g < (int)mark.size() && mark[(size_t)g]; }
    void add(int g) {
        if (!mark[(size_t)g]) {
            mark[(size_t)g] = 1;
            items.push_back(g);
        }
    }
    void clear() {
        for (int g : items) mark[(size_t)g] = 0;
        items.clear();
    }
    bool empty() const { return items.empty(); }
};

// a dict tile -> int that is never deleted from (keys kept in insertion order), cleared in O(size)
struct IntMap {
    std::vector<int> val;
    std::vector<int> keys;
    void init(int n) {
        val.assign((size_t)n, NOVAL);
        keys.clear();
    }
    int get(int g) const { return (g >= 0 && g < (int)val.size()) ? val[(size_t)g] : NOVAL; }
    bool has(int g) const { return get(g) != NOVAL; }
    void set(int g, int v) {
        if (val[(size_t)g] == NOVAL) keys.push_back(g);
        val[(size_t)g] = v;
    }
    void clear() {
        for (int g : keys) val[(size_t)g] = NOVAL;
        keys.clear();
    }
    size_t size() const { return keys.size(); }
};

// a dict int -> int with Python's iteration order (a key deleted and set again goes to the back)
struct OrderedMap {
    std::vector<int> val;
    std::vector<char> in;
    std::vector<uint64_t> seq;
    uint64_t ctr = 0;
    size_t n = 0;
    void init(int size) {
        val.assign((size_t)size, 0);
        in.assign((size_t)size, 0);
        seq.assign((size_t)size, 0);
        ctr = 0;
        n = 0;
    }
    bool has(int k) const { return in[(size_t)k] != 0; }
    int get(int k, int def) const { return in[(size_t)k] ? val[(size_t)k] : def; }
    void set(int k, int v) {
        if (!in[(size_t)k]) {
            in[(size_t)k] = 1;
            seq[(size_t)k] = ++ctr;
            n++;
        }
        val[(size_t)k] = v;
    }
    void erase(int k) {
        if (in[(size_t)k]) {
            in[(size_t)k] = 0;
            n--;
        }
    }
    std::vector<std::pair<int, int>> items() const {
        std::vector<std::pair<uint64_t, int>> ks;
        for (size_t k = 0; k < in.size(); k++)
            if (in[k]) ks.push_back({seq[k], (int)k});
        std::sort(ks.begin(), ks.end());
        std::vector<std::pair<int, int>> out;
        for (auto& e : ks) out.push_back({e.second, val[(size_t)e.second]});
        return out;
    }
};

// a small dict int -> int (a few dozen keys at most: the dragons in view)
struct SmallMap {
    std::vector<std::pair<int, int>> kv;
    void clear() { kv.clear(); }
    const int* find(int k) const {
        for (auto& e : kv)
            if (e.first == k) return &e.second;
        return nullptr;
    }
    bool has(int k) const { return find(k) != nullptr; }
    int get(int k, int def) const {
        const int* p = find(k);
        return p ? *p : def;
    }
    void set(int k, int v) {
        for (auto& e : kv)
            if (e.first == k) {
                e.second = v;
                return;
            }
        kv.push_back({k, v});
    }
};

// ptiles[tile]: {direction: portal id}, directions in insertion order (PORTAL_PERIOD iterates them)
struct PTile {
    int pid[4] = {0, 0, 0, 0};
    bool has[4] = {false, false, false, false};
    int order[4] = {0, 0, 0, 0};
    int n = 0;
    void set(int d, int p) {
        if (!has[d]) {
            has[d] = true;
            order[n++] = d;
        }
        pid[d] = p;
    }
    int get(int d, int def) const { return has[d] ? pid[d] : def; }
};

struct Tri {          // (tile, round, length / segments)
    int a, b, c;
};

struct Head {         // (window index, id, is_enemy)
    int i, did;
    bool en;
};

struct Enemy {        // (window index, id, visible length / what I vouch for, estimated length)
    int ei, did, ln, est;
};

struct Scored {       // [score, direction, window index, global tile, kind]
    double s;
    int d, v, g2, kind;
};

struct Cand {
    int d, v, g2, kind;
};

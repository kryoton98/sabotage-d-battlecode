// Sabotage-d / base.hpp: what the port stands on - Python's arithmetic and parsing (fdiv, fmod, pyint, parse_msg,
// ...), the RNG, and the static tables.
// A piece of main.cpp: included once, inside its namespace. Not a header to include from anywhere else.

// ------------------------------------------------------------------------------------------------ Python semantics
struct PyError : std::runtime_error {
    explicit PyError(const char* what) : std::runtime_error(what) {}
};

inline long long fdiv(long long a, long long b) {       // Python a // b
    long long q = a / b;
    if ((a % b) != 0 && ((a < 0) != (b < 0))) --q;
    return q;
}
inline long long fmod(long long a, long long b) {       // Python a % b
    long long r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) r += b;
    return r;
}
inline int iabs(int a) { return a < 0 ? -a : a; }
inline bool T(double x) { return x != 0.0; }            // Python truthiness of a number

inline bool is_space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

// int(s): optional surrounding whitespace, an optional sign, decimal digits (ValueError otherwise)
long long pyint(const std::string& s) {
    size_t i = 0, n = s.size();
    while (i < n && is_space(s[i])) i++;
    while (n > i && is_space(s[n - 1])) n--;
    if (i >= n) throw PyError("int(): empty");
    bool neg = false;
    if (s[i] == '+' || s[i] == '-') {
        neg = s[i] == '-';
        i++;
    }
    if (i >= n) throw PyError("int(): sign only");
    long long v = 0;
    for (; i < n; i++) {
        char c = s[i];
        if (c < '0' || c > '9') throw PyError("int(): not a digit");
        v = v * 10 + (c - '0');
    }
    return neg ? -v : v;
}

// int(m) of a sonar value, which may be anything a dragon sent: st 0 = not a number, -1 = negative, 1 = a value
// (big: at least 2^64, larger than any bound the readers accept)
struct MsgVal {
    int st;
    bool big;
    uint64_t v;
};
MsgVal parse_msg(const std::string& s) {
    MsgVal r{0, false, 0};
    size_t i = 0, n = s.size();
    while (i < n && is_space(s[i])) i++;
    while (n > i && is_space(s[n - 1])) n--;
    if (i >= n) return r;
    bool neg = false;
    if (s[i] == '+' || s[i] == '-') {
        neg = s[i] == '-';
        i++;
    }
    if (i >= n) return r;
    bool nonzero = false;
    for (; i < n; i++) {
        char c = s[i];
        if (c < '0' || c > '9') return MsgVal{0, false, 0};
        unsigned d = (unsigned)(c - '0');
        if (d) nonzero = true;
        if (!r.big) {
            if (r.v > (UINT64_MAX - d) / 10) r.big = true;
            else r.v = r.v * 10 + d;
        }
    }
    r.st = (neg && (nonzero || r.big)) ? -1 : 1;
    if (r.st == 1 && neg) r.v = 0;   // "-0"
    return r;
}

std::vector<std::string> split_ws(const std::string& s) {     // str.split()
    std::vector<std::string> out;
    size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && is_space(s[i])) i++;
        if (i >= n) break;
        size_t j = i;
        while (j < n && !is_space(s[j])) j++;
        out.emplace_back(s, i, j - i);
        i = j;
    }
    return out;
}

std::string tail_from(const std::string& s, size_t k) { return k < s.size() ? s.substr(k) : std::string(); }  // s[k:]

bool starts_with(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }

int didx(char c) {                                     // DIDX
    switch (c) {
        case 'N': return 0;
        case 'E': return 1;
        case 'S': return 2;
        case 'W': return 3;
    }
    throw PyError("DIDX: not a direction");
}
int didx_s(const std::string& s) {                     // DIDX[s] (KeyError unless exactly one of N E S W)
    if (s.size() != 1) throw PyError("DIDX: not a direction");
    return didx(s[0]);
}

const std::string& at(const std::vector<std::string>& L, long long i) {   // L[i] (IndexError past the end)
    if (i < 0 || i >= (long long)L.size()) throw PyError("list index out of range");
    return L[(size_t)i];
}

// ------------------------------------------------------------------------------------------------ the RNG of PORTING.md
uint64_t g_rs = 0;
double rand01() {                                      // _lcg
    uint64_t x = g_rs * 6364136223846793005ULL + 1442695040888963407ULL;
    g_rs = x;
    return (double)(x >> 11) * (1.0 / 9007199254740992.0);
}

// ------------------------------------------------------------------------------------------------ static tables
const char DCH[5] = "NESW";
const int OPP[4] = {2, 3, 0, 1};
const int DXS[4] = {0, 1, 0, -1};
const int DYS[4] = {-1, 0, 1, 0};
const int NEVER = 9999;
const int UNSEEN = -100000;
const int BARREN_SEEN = 1000000000;
const char* ALL_OPEN_H = ". . . . . . .";
const char* ALL_OPEN_V = ". . . . . . . .";

int BASE[196];        // BASE[i*4+d]: neighbour of window index i in direction d, -2 = leaves the window
int RING[49];
int ROWS[49];
int COLS[49];
std::vector<std::pair<int, int>> CLUSTER_OFF;
std::vector<std::pair<int, int>> DENSE_OFF;

void init_tables() {
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 7; c++)
            for (int d = 0; d < 4; d++) {
                int rr = r + DYS[d], cc = c + DXS[d];
                BASE[(r * 7 + c) * 4 + d] = (0 <= rr && rr < 7 && 0 <= cc && cc < 7) ? rr * 7 + cc : -2;
            }
    for (int i = 0; i < 49; i++) {
        int r = i / 7, c = i % 7;
        RING[i] = (r == 0 || r == 6 || c == 0 || c == 6) ? 1 : 0;
        ROWS[i] = r;
        COLS[i] = c;
    }
    for (int ox = -2; ox < 3; ox++)
        for (int oy = -2; oy < 3; oy++) {
            int m = iabs(ox) + iabs(oy);
            if (0 < m && m <= 2) CLUSTER_OFF.push_back({ox, oy});
            if (ox || oy) DENSE_OFF.push_back({ox, oy});
        }
}

// Modular dragon bot with bounded movement and body-aware continuation searches.
// The queen avoids portal transitions on boards of at least 1,100 tiles,
// allowing a portal only when the checked ordinary routes offer no viable escape.
// QUEEN_RETREAT enables a queen-only escape search when any enemy part is visible.
// Its search checks our moving body and known terrain; future enemy moves remain uncertain.
// Worker roles, communication, growth and late-game gathering use the existing strategy.
// Default switches: QUEEN_PORTAL_LAST = 1, QUEEN_RETREAT = 1.

#include "queen_safety.hpp"
#include "body_search.hpp"
#include "traversal.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <functional>
#include <tuple>

namespace {

#include "base.hpp"         // Python semantics (fdiv, fmod, pyint, ...), the RNG, the static tables
#include "params.hpp"       // struct Params: every tunable and its default (+ known_maps_data.hpp: the ladder maps)
#include "param_names.hpp"  // parameter names for the JSON overrides
#include "containers.hpp"   // TileSet, IntMap, OrderedMap, SmallMap, PTile; Head, Enemy, Scored, Cand
#include "brain.hpp"        // struct Brain: the state, and the includes of everything it does
#include "params_json.hpp"  // apply_params: overrides for Params from argv[1]

// ------------------------------------------------------------------------------------------------ stdin / stdout
// sys.stdin.readline(): the next line with its "\n", "" at the end of input.  (fgets, not fread: fread on a pipe
// waits until its whole buffer is full, and the engine sends one turn at a time.)
struct LineReader {
    char buf[4096];
    std::string rl() {
        std::string out;
        while (fgets(buf, sizeof buf, stdin) != nullptr) {
            out += buf;
            if (!out.empty() && out.back() == '\n') break;
        }
        // universal newlines: "\r\n" reads as "\n"
        if (out.size() >= 2 && out[out.size() - 2] == '\r' && out.back() == '\n') out.erase(out.size() - 2, 1);
        return out;
    }
};

std::string rstrip_nl(const std::string& s) {        // s.rstrip("\n")
    size_t n = s.size();
    while (n > 0 && s[n - 1] == '\n') n--;
    return s.substr(0, n);
}

}  // namespace

int main(int argc, char** argv) {
    uint64_t turn_epoch=traversal::ticks();
    // one buffer, one write a turn: output is priced per write
    setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
    init_tables();
    Params params;
    if (argc > 1 && argv[1][0]) {
        try {
            apply_params(params, argv[1]);
        } catch (const std::exception& ex) {
            fprintf(stderr, "bad parameters: %s\n", ex.what());
            return 2;
        }
    }
    LineReader in;
    // init block: ID / TEAM / MAP w h / UNIT_LIMIT, read by key until the first ROUND line
    int did = 0, w = 0, h = 0, limit = 64;
    std::string team = "A";
    std::string line = in.rl();
    while (!line.empty() && !starts_with(line, "ROUND")) {
        auto p = split_ws(line);
        if (p.size() >= 2) {
            if (p[0] == "ID") did = (int)pyint(p[1]);
            else if (p[0] == "TEAM") team = p[1];
            else if (p[0] == "MAP" && p.size() >= 3) {
                w = (int)pyint(p[1]);
                h = (int)pyint(p[2]);
            } else if (p[0] == "UNIT_LIMIT") limit = (int)pyint(p[1]);
        }
        line = in.rl();
    }
    if (w <= 0 || h <= 0) return 1;
    Brain brain(did, team, w, h, limit, params);
    while (true) {
        while (line == "\n") line = in.rl();
        if (line.empty() || line[0] == 'E') return 0;   // EOF or ENDGAME
        std::vector<std::string> L{line, in.rl(), in.rl(), in.rl(), in.rl()};
        std::string reply;
        try {
            long long nm = pyint(tail_from(L[4], 9));
            for (long long k = 0; k < nm; k++) L.push_back(in.rl());   // the sonar messages
            std::string nxt = in.rl();                                  // "ECHOES ..." or the first tile line
            L.push_back(nxt);
            int more = starts_with(nxt, "ECHOES") ? 49 : 48;
            for (int k = 0; k < more; k++) L.push_back(in.rl());
            std::string hdr = in.rl();
            L.push_back(hdr);
            long long nb = pyint(tail_from(hdr, 14));
            for (long long k = 0; k < nb + 15; k++) L.push_back(in.rl());
            for (auto& s : L) s = rstrip_nl(s);
            brain.traversal_epoch=turn_epoch;
            brain.body_budget={turn_epoch,0,false};
            reply = brain.turn(L);
        } catch (const std::exception& ex) {
            if (T(params.DEBUG)) {
                // (DEBUG: an exception is a bug to see, not to paper over - tools/sweep_cpp.py counts these)
                fprintf(stderr, "EXCEPTION (%s): %s\n", rstrip_nl(line).c_str(), ex.what());
                return 3;
            }
            std::string d = L.size() > 1 ? tail_from(L[1], 4).substr(0, 1) : std::string();
            reply = std::string("MOVE ") + ((d == "N" || d == "E" || d == "S" || d == "W") ? d : std::string("N"));
        }
        reply += "\nENDTURN\n";
        fwrite(reply.data(), 1, reply.size(), stdout);   // one write + one flush per turn
        fflush(stdout);
        turn_epoch=traversal::ticks();
        line = in.rl();
    }
}

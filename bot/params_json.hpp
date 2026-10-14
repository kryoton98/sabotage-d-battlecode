// Sabotage-d / params_json.hpp: Json and apply_params - overrides for Params from the first command-line argument
// (local tests only: the judge passes none).
// A piece of main.cpp: included once, inside its namespace. Not a header to include from anywhere else.

// ------------------------------------------------------------------------------------------------ parameters from JSON
// A tiny JSON reader for the override object: {"NAME": number | true | false | null, "CHEAT_PORTALS": [[k, g, p], ...]}
struct Json {
    const char* p;
    const char* e;
    void ws() {
        while (p < e && is_space(*p)) p++;
    }
    bool lit(const char* s) {
        size_t n = strlen(s);
        if ((size_t)(e - p) >= n && strncmp(p, s, n) == 0) {
            p += n;
            return true;
        }
        return false;
    }
    std::string str() {
        std::string out;
        if (p >= e || *p != '"') throw PyError("json: string expected");
        p++;
        while (p < e && *p != '"') {
            if (*p == '\\' && p + 1 < e) p++;
            out += *p++;
        }
        if (p >= e) throw PyError("json: unterminated string");
        p++;
        return out;
    }
    double num() {
        char* end = nullptr;
        double v = strtod(p, &end);
        if (end == p) throw PyError("json: number expected");
        p = end;
        return v;
    }
    // a value: numbers / booleans / null -> *out; arrays of arrays of numbers -> *arr
    void value(double* out, bool* isnull, std::vector<std::vector<double>>* arr) {
        ws();
        if (lit("true")) {
            *out = 1;
        } else if (lit("false")) {
            *out = 0;
        } else if (lit("null")) {
            *isnull = true;
        } else if (p < e && *p == '[') {
            p++;
            ws();
            while (p < e && *p != ']') {
                ws();
                std::vector<double> row;
                if (p < e && *p == '[') {
                    p++;
                    ws();
                    while (p < e && *p != ']') {
                        ws();
                        row.push_back(num());
                        ws();
                        if (p < e && *p == ',') p++;
                        ws();
                    }
                    p++;
                } else {
                    row.push_back(num());
                }
                arr->push_back(row);
                ws();
                if (p < e && *p == ',') p++;
                ws();
            }
            p++;
        } else if (p < e && *p == '"') {
            str();                      // a string value: no parameter takes one
        } else {
            *out = num();
        }
    }
};

void apply_params(Params& P, const std::string& text) {
    Json j{text.data(), text.data() + text.size()};
    j.ws();
    if (j.p >= j.e || *j.p != '{') throw PyError("params: a JSON object expected");
    j.p++;
    while (true) {
        j.ws();
        if (j.p < j.e && *j.p == '}') break;
        std::string key = j.str();
        j.ws();
        if (j.p >= j.e || *j.p != ':') throw PyError("params: ':' expected");
        j.p++;
        double v = 0;
        bool isnull = false;
        std::vector<std::vector<double>> arr;
        j.value(&v, &isnull, &arr);
        size_t colon = key.find(':');
        if (colon != std::string::npos && colon >= 2 && (key[0] == 'A' || key[0] == 'B')) {
            // (7 Oct) an area override: "A4000:NAME" (maps of 4000+ tiles) / "B1100:NAME" (maps under 1100 tiles)
            const double area = atof(key.substr(1, colon - 1).c_str());
            const std::string nm = key.substr(colon + 1);
            bool known = false;
            for (auto& pn : PARAM_NAMES)
                if (nm == pn.name) {
                    P.area_ovr.push_back({area, key[0] == 'B' ? 1 : 0, pn.field, isnull ? 0.0 : v});
                    known = true;
                }
            if (!known) fprintf(stderr, "unknown parameter %s (ignored)\n", key.c_str());
        } else if (colon != std::string::npos && colon >= 2 && key[0] == 'M' && key[1] >= '0' && key[1] <= '9') {
            // (8 Oct, ORA) a known-map override: "M<k>:NAME" (KMAPS index k)
            const int k = atoi(key.substr(1, colon - 1).c_str());
            const std::string nm = key.substr(colon + 1);
            bool known = false;
            for (auto& pn : PARAM_NAMES)
                if (nm == pn.name) {
                    P.map_ovr.push_back({k, pn.field, isnull ? 0.0 : v});
                    known = true;
                }
            if (!known) fprintf(stderr, "unknown parameter %s (ignored)\n", key.c_str());
        } else if (key == "CHEAT_PORTALS") {
            P.cheat_portals.clear();
            for (auto& row : arr)
                if (row.size() >= 3) P.cheat_portals.push_back({(int)row[0], (int)row[1], (int)row[2]});
        } else {
            bool known = false;
            for (auto& pn : PARAM_NAMES)
                if (key == pn.name) {
                    P.*(pn.field) = isnull ? 0.0 : v;
                    known = true;
                }
            if (!known) fprintf(stderr, "unknown parameter %s (ignored)\n", key.c_str());
        }
        j.ws();
        if (j.p < j.e && *j.p == ',') {
            j.p++;
            continue;
        }
        break;
    }
}

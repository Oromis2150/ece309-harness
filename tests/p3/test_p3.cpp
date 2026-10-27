// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
}

// ------------------------------------------------------------- your tests
//
// Still to write (spec §6 has the full list). Suggested names:
//   reg_null_rejected, reg_for_each_visits_each_once
//   reg_10000_tools               REQUIRED: print bucket_count / load_factor /
//                                 max_probe_length, check the bound
//   reg_alternative_hash          REQUIRED: longest chain of a weak hash
//   ws_<case>                     REQUIRED: every row of the table in spec §4.4
//   calc_<case>                   left-to-right, parentheses, negatives

TEST(reg_null_and_empty) {
    ToolRegistry r; CHECK(r.size() == 0); CHECK(r.bucket_count() > 0);
    CHECK(r.max_probe_length() == 0); CHECK(!r.add(nullptr)); CHECK(r.size() == 0);
}
TEST(reg_for_each) {
    ToolRegistry r; for (int i = 0; i < 5; ++i) REQUIRE(r.add(std::make_unique<NamedTool>(std::to_string(i))));
    std::vector<int> counts(5); r.for_each([&](const Tool& t) { ++counts[std::stoi(t.name())]; });
    for (int n : counts) CHECK(n == 1);
}
TEST(reg_scale_and_weak_hash) {
    ToolRegistry r; Tool* first = nullptr;
    for (int i = 0; i < 10000; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i))));
        if (i == 0) first = r.find("tool_0");
        CHECK(r.load_factor() <= 1.0);
    }
    CHECK(first == r.find("tool_0"));
    for (int i = 0; i < 10000; ++i) CHECK(r.find("tool_" + std::to_string(i)) != nullptr);
    CHECK(r.max_probe_length() <= 64);
    std::vector<int> counts(r.bucket_count());
    for (int i = 0; i < 10000; ++i) { auto s = "tool_" + std::to_string(i); ++counts[(static_cast<unsigned char>(s[0]) + s.size()) % counts.size()]; }
    int longest = 0; for (int n : counts) if (n > longest) longest = n;
    std::printf("buckets=%zu load=%.6f max_chain=%zu weak_chain=%d\n", r.bucket_count(), r.load_factor(), r.max_probe_length(), longest);
}
TEST(reg_hash_order) { CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba")); }
TEST(ws_normal_paths) {
    TempDir d; Workspace ws(d.path()); fs::path out; std::string why;
    const std::vector<std::pair<std::string,std::string>> cases = {
      {"notes/todo.txt", "notes/todo.txt"}, {"./notes//todo.txt", "notes/todo.txt"},
      {".", ""}, {"a/../b", "b"}, {"a/..", ""},
      {"new/dir/file.txt", "new/dir/file.txt"}, {"my..notes.txt", "my..notes.txt"}};
    for (const auto& c : cases) { REQUIRE(ws.resolve(c.first, &out, &why)); CHECK(out == (c.second.empty() ? ws.root() : ws.root() / c.second)); }
}
TEST(ws_invalid_paths) {
    TempDir d; Workspace ws(d.path()); fs::path out; std::string why;
    std::vector<std::string> cases = {"", std::string("notes") + '\0' + "/x", "/etc/passwd", "..", "../etc/passwd", "a/../../b", "notes/../../ws/notes/todo.txt"};
    for (const auto& c : cases) { why.clear(); CHECK(!ws.resolve(c, &out, &why)); CHECK(!why.empty()); }
}
TEST(calc_subtract_left_to_right) { double v; std::string e; REQUIRE(eval("10-4-3", &v, &e)); CHECK(v == 3); }
TEST(calc_divide_left_to_right) { double v; std::string e; REQUIRE(eval("100/10/5", &v, &e)); CHECK(v == 2); }
TEST(calc_parentheses) { double v; std::string e; REQUIRE(eval("(2+3)*4", &v, &e)); CHECK(v == 20); }
TEST(calc_trace_example) { double v; std::string e; REQUIRE(eval("2*(3+4)-5", &v, &e)); CHECK(v == 9); }
TEST(calc_negatives) { double v; std::string e; REQUIRE(eval("-3*-3", &v, &e)); CHECK(v == 9); }
TEST(calc_fraction) { double v; std::string e; REQUIRE(eval("7/2", &v, &e)); CHECK(v == 3.5); }
TEST(calc_nested_zero) { double v; std::string e; CHECK(!eval("1/(2-2)", &v, &e)); CHECK(e == "division by zero"); }
TEST(calc_extra) { double v; std::string e; REQUIRE(eval("3.5+2.5*4", &v, &e)); CHECK(v == 13.5); REQUIRE(eval("8/(2+2)+6", &v, &e)); CHECK(v == 8); }
TEST(calc_invalid_tokens) { double v; std::string e; CHECK(!calc_evaluate({}, &v, &e)); CHECK(!e.empty()); std::vector<CalcToken> t{{CalcToken::Kind::RParen, 0}}; CHECK(!calc_evaluate(t, &v, &e)); }
TEST(calc_large_inputs) { double v; std::string e; std::string s = "1"; for (int i=1;i<20000;++i) s += "+1"; REQUIRE(eval(s,&v,&e)); CHECK(v == 20000); s = std::string(5000,'(') + "1" + std::string(5000,')'); REQUIRE(eval(s,&v,&e)); CHECK(v == 1); }

int main(int argc, char** argv) { return mt::run_all(argc, argv); }

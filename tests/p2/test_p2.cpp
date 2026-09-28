// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.
//
// This file is a stub so the project builds out of the box; replace the
// body of main() with your own tests.

#undef NDEBUG
#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>
#include <iostream>
#include <utility>
#include <memory>
#include <string>
#include <stdexcept>
#include <fstream>
#include <vector>

#define TEST(name) static void name()
#define RUN(name) do { name(); std::cout << "PASS " #name "\n"; } while (0)

static const std::string SENTINEL = "<|end_conversation|>";

// ---------------------------- Helpers -------------------------------------------------------
static bool same(const Message& x, const Message& y) {
    return x.role() == y.role() && x.content() == y.content();
}

static bool same_conv(const Conversation& a, const Conversation& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!same(a.at(i), b.at(i))) return false;
    }
    return true;
}

static Message msg(std::size_t i) {
    return Message(i % 2 ? Role::Assistant : Role::User, "message #" + std::to_string(i));
}

struct ScanResult {std::string safe; bool found; };

static ScanResult scan_pieces(const std::vector<std::string>& pieces) {
    SentinelScanner s(SENTINEL);
    ScanResult r{"", false};
    for (const auto& p : pieces) {
        auto o = s.feed(p);
        r.safe += o.safe_text;
        r.found = r.found || o.sentinel_found;
    }
    if (!r.found) r.safe += s.flush().safe_text;
    return r;
}

// -------------------------------Message/Conversation Tests --------------------------------------------

TEST(MessageDefaultIsEmptySystem) {
    Message m;
    assert(m.role() == Role::System);
    assert(m.content().empty());
}

TEST(EmptyConversationBounds) {
    Conversation c;
    assert(c.size() == 0);
    assert(c.begin() == c.end());

    bool threw = false;
    try { (void)c.at(0); } catch (const std::out_of_range&) { threw = true; }
    assert(threw && "at(0) on empty conversation must throw");

    int n = 0;
    for (const Message&m : c) { (void)m; ++n; }
    assert(n == 0);
}

TEST(AtIsBoundsChecked) {
    Conversation c;
    for (std::size_t i = 0; i < 5; ++i) c.append(msg(i));
    assert(same(c.at(4), msg(4)));

    bool threw = false;
    try { (void)c.at(5); } catch (const std::out_of_range&) { threw = true; }
    assert(threw && "at(size()) must throw");

    threw = false;
    try { (void)c.at(static_cast<std::size_t>(-1)); }
    catch (const std::out_of_range&) { threw = true; }
    assert(threw && "at(huge) must throw");
}

TEST(SystemMessageStaysFirst) {
    Conversation c;
    c.append(Message(Role::System, "Be concise."));
    for (std::size_t i = 0; i < 100; ++i) c.append(msg(i));

    assert(c.at(0).role() == Role::System);
    assert(c.at(0).content() == "Be concise.");
    assert(c.begin()->role() == Role::System);

    // Survives Copy and Move
    Conversation copy(c);
    assert(copy.at(0).role() == Role::System && copy.at(0).content() == "Be concise.");
    Conversation moved(std::move(c));
    assert(moved.at(0).role() == Role::System && moved.at(0).content() == "Be concise.");

    for (std::size_t i = 0; i < 100; ++i) assert(same(moved.at(i + 1), msg(i)));
}

TEST(ConversationCopyIsDeep) {
    Conversation a;
    a.append(Message(Role::User, "Hello"));
    a.append(Message(Role::Assistant, "How can I help you?"));
    Conversation b(a);

    assert(b.begin() != a.begin()); // Different buffers
    assert(same_conv(a, b));

    b.append(Message(Role::System, "Notification"));
    assert(a.size() == 2 && b.size() == 3);
    assert(same(a.at(1), Message(Role::Assistant, "How can I help you?")));

    Conversation* heap = new Conversation(a);
    delete heap;
    assert(same_conv(a,b) == false); // b should have an extra message   
}

TEST(CopyAssignmentAndCopyOfEmpty) {
    Conversation empty;
    Conversation e2(empty);
    assert(e2.size() == 0 && e2.begin() == e2.end());

    Conversation a;
    for (std::size_t i = 0; i < 7; ++i) {
        a.append(msg(i));
    }

    Conversation b;
    for (std::size_t i = 0; i < 3; ++i) {
        b.append(msg(100 + i));
    }
    b = a;
    assert(b.begin() != a.begin());
    assert(same_conv(a, b));

    Conversation& self = b;
    b = self;
    assert(b.size() == 7 && same_conv(a, b));

    b = empty;
    assert(b.size() == 0 && b.begin() == b.end());
}

TEST(GrowthAndReallocation) {
    // Doubling capacity

    auto is_realloc_point = [](std::size_t n) { return n == 0 || (n & (n-1)) == 0; };

    Conversation c;
    const Message* prev = c.begin();
    std::size_t reallocs = 0;
    const std::size_t N = 1000;

    for (std::size_t i = 0; i < N; ++i) {
        c.append(msg(i));
        const Message* now = c.begin();
        bool moved = (now != prev);
        assert(moved == is_realloc_point(i) && "buffer must move iff size==capacity before the append");
        
        if (moved) ++reallocs;
        prev = now;

        assert(c.size() == i + 1);
        assert(same(c.at(i), msg(i)));
        assert(same(c.at(0), msg(0)));
    }

    for (std::size_t i = 0; i < N; ++i) assert(same(c.at(i), msg(i)));

    assert(reallocs == 11);

    std::size_t k = 0;
    for (const Message& m : c) assert(same(m, msg(k++)));
    assert(k == N);
    assert(c.end() - c.begin() == static_cast<std::ptrdiff_t>(N));
}

TEST(ConversationMoveStealsBuffer) {
    Conversation a;
    for (std::size_t i = 0; i < 10; ++i) a.append(msg(i));
    const Message* original_ptr = a.begin();

    Conversation b(std::move(a));
    assert(b.begin() == original_ptr && "move ctor must steal the pointer");
    assert(b.size() == 10);

    // Moved-from object must be valid and empty
    assert(a.size() == 0);
    assert(a.begin() == nullptr && a.end() == nullptr);
    a.append(msg(42));
    assert(a.size() == 1 && same(a.at(0), msg(42)));

    // Move assignment must steal and free the target's old buffer
    Conversation c;
    for (std::size_t i = 0; i < 4; ++i) c.append(msg(200 + i));
    const Message* b_ptr = b.begin();
    c = std::move(b);
    assert(c.begin() == b_ptr && c.size() == 10);
    assert(b.size() == 0 && b.begin() == nullptr);

    // Self-move must not destroy data
    Conversation& cref = c;
    c = std::move(cref);
    assert(c.size() == 10 && same(c.at(9), msg(9)));
}

// ---------------------------------SentinelScanner Tests-----------------------------------------

TEST(ScannerCleanText) {
    auto r = scan_pieces({"Hello world. Nothing to see here."});
    assert(!r.found);
    assert(r.safe == "Hello world. Nothing to see here.");

    auto r2 = scan_pieces({"", "abc", ""});
    assert(!r2.found && r2.safe == "abc");
    auto r3 = scan_pieces({});
    assert(!r3.found && r3.safe.empty());
}

TEST(ScanWholeSentinelInOneChunk) {
    SentinelScanner s(SENTINEL);
    auto out = s.feed("Goodbye." + SENTINEL);
    assert(out.sentinel_found);
    assert(out.safe_text == "Goodbye.");
}

TEST(ScanSentinelSplitIntoThree) {
    const std::string text = "Bye." + SENTINEL;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        for (std::size_t j = 0; j < text.size(); ++j) {
            SentinelScanner s(SENTINEL);
            std::string safe;
            bool found = false;

            for (const std::string& piece : {text.substr(0,i), text.substr(i, j - i), text.substr(j)}) {
                auto o = s.feed(piece);
                safe += o.safe_text;
                found = found || o.sentinel_found;
            }
            assert(found && safe == "Bye.");
        }
    }
}

TEST(ScanOneCharAtATime) {
    const std::string text = "Goodbye." + SENTINEL;
    SentinelScanner s(SENTINEL);
    std::string safe;
    std::size_t found_at = std::string::npos;

    for (std::size_t i = 0; i < text.size(); ++i) {
        auto o = s.feed(std::string_view(&text[i], 1));
        safe += o.safe_text;
        if (o.sentinel_found) { found_at = i; break; }
    }
    assert(found_at == text.size() - 1 && "must report on the final '>' exactly");
    assert(safe == "Goodbye.");
}

int main() {

    // Conversation Tests
    RUN(MessageDefaultIsEmptySystem);
    RUN(EmptyConversationBounds);
    RUN(SystemMessageStaysFirst);
    RUN(AtIsBoundsChecked);
    RUN(ConversationCopyIsDeep);
    RUN(GrowthAndReallocation);
    RUN(CopyAssignmentAndCopyOfEmpty);
    RUN(ConversationMoveStealsBuffer);

    // Scanner Tests
    RUN(ScannerCleanText);
    RUN(ScanWholeSentinelInOneChunk);
    RUN(ScanSentinelSplitIntoThree);
    RUN(ScanOneCharAtATime);

    // TODO: write your tests here.
    return 0;
}

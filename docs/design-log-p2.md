# Design Log — Project 2: The Conversation Loop

## 1. Growth factor and amortized O(1) append

**Doubling:** `append` reallocates only when `size_ == capacity_`, and the new capacity is `capacity_ == 0 ? 1 : capacity_ * 2`, so capacity runs 0 → 1 → 2 → 4 → 8 → …

**Cost model:** One unit is one `Message` write. Moving a `Message` is O(1) because `std::string`'s move steals its heap buffer. Reallocating a full array of `c` elements therefore costs `c` units, and every append costs 1 unit for its own write.

**Proof:** Take `n` appends starting from an empty conversation. Before append `i` (0-indexed), `size_ = i`. A reallocation happens exactly when `i` is 0 or a power of two, and `i ≤ n − 1`. Let `2^k` be the largest such power, so `2^k ≤ n − 1`. The reallocations move `0 + 1 + 2 + 4 + … + 2^k` elements. That is a geometric series:

```
sum_{j=0..k} 2^j = 2^(k+1) − 1  <  2 · 2^k  ≤  2(n − 1)  <  2n
```

Total work is at most `n` (writes) `+ 2n` (moves) `= 3n`, so the amortized cost per append is at most 3, which is O(1).

**Accounting:** Charge every append 3 credits. One credit pays for its own write and two are banked. When the array grows to capacity `c`, it holds `c/2` elements, so the next `c/2` appends bank `2 · c/2 = c` credits. That is exactly what the next reallocation costs, so the account never goes negative.

**Why Not A Fixed Increment:** Growing by +1 reallocates on every append, costing `1 + 2 + … + n = n(n+1)/2 = O(n²)`.

**Trade-offs:** Unused capacity is always under 50% of the buffer, which is negligible at chat-history sizes. A single append can still cost O(n), but that happens only at powers of two. The growth test observes the buffer address changing at exactly those sizes: 11 reallocations over 1000 appends.

## 2. Rule of Five safety

`Conversation` owns one `Message[]` through `data_`. It is the only class that uses raw `new`/`delete`, and it stores `Message` by value rather than `Message*`, so ownership is never ambiguous.

- **Destructor:** `delete[] data_`. Deleting `nullptr` is a no-op, so empty and moved-from objects are safe.
- **Copy constructor (deep):** allocates a fresh buffer of `other.capacity_` and copy-assigns each of the `size_` messages. It never copies the pointer, so no two objects share a buffer and there is no double-free. A test asserts `b.begin() != a.begin()`, and that appending to the copy leaves the original unchanged.
- **Copy assignment:** copy-and-swap. It builds a temporary copy, then swaps with `noexcept swap`. This handles self-assignment correctly and leaves `*this` untouched if the copy throws (strong guarantee). The old buffer is released when the temporary is destroyed.
- **Move constructor:** copies the three fields, then sets `other.data_ = nullptr` and `other.size_ = other.capacity_ = 0`. No element is touched. It is `noexcept`, and a test asserts `b.begin()` equals the original pointer.
- **Move assignment:** guards against `this == &other`, frees the current buffer, steals the source's fields, and zeroes the source. It is `noexcept`.
- **Moved-from state:** `data_ == nullptr` and `size_ == capacity_ == 0`. That is identical to a default-constructed object, so it can be destroyed, reassigned, or appended to (the next `append` allocates capacity 1 again).
- **Reallocation:** `append` builds the new buffer, moves elements into it, and only then does `delete[] data_`. The old buffer is freed exactly once.

All tests run under AddressSanitizer (`-fsanitize=address`). Any leak, double-free, or use-after-free in these paths fails the build.

## 3. Bounded `pending_` buffer

Let `m = sentinel_.size() ≥ 1`, let `p` be `pending_.size()` between calls, and let `c` be the size of the incoming chunk.

**Invariant:** between calls, `p ≤ m − 1`.

- **Base:** `pending_` starts empty, so `p = 0 ≤ m − 1`.
- **Step:** `feed` appends the chunk, so `pending_` briefly holds `p + c` characters. There are two cases.
  1. **Sentinel found:** `pending_.clear()` runs, so `p = 0`.
  2. **Not found:** `keep = min(m − 1, p + c)`. The code erases `(p + c) − keep` characters, leaving exactly `keep ≤ m − 1`.
- `flush()` clears `pending_`, so `p = 0`.

By induction the invariant holds after every call. Inside a call, `pending_` never exceeds `(m − 1) + c` characters. That is bounded by the chunk the caller already holds and does not depend on the total stream length `N`.

**Emitter Safety:** Suppose no full sentinel occurs in `pending_`. Any occurrence that is not yet complete must start within the last `m − 1` characters. If it started earlier, all `m` of its characters would already be in `pending_` and `find` would have found it. So everything before the last `m − 1` characters cannot belong to any future match, and can be printed immediately.

**Cost.** Each call scans at most `(m − 1) + c` characters, so the total scan over a stream of `N` characters is `N + (m − 1) · (number of calls)`. With `m` fixed at 20, that is O(N) even for one-byte chunks. Concatenating everything and re-searching from the start would cost O(N²) time and O(N) space. The stress test feeds 4 MB one byte at a time, measures `total_fed − total_emitted` after every call, and asserts it never exceeds 19.

**In Hindsight:** Firstly, I should have begun this project much sooner. I found myself scrambling to review the provided code and debugging my own. So, I would design a better work schedule for myself: designating recurring times to work on the project throughout a week, rather than doing it in the last few days.
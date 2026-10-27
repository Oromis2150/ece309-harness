# Project 3 Design Log
**Author:** Donovan Reyes <br> **P2 code used:** My Own <br>
<!-- 500–800 words. An argument, not a diary: say what you decided, what
     you rejected, and what evidence backs it up. Delete these comments
     before submitting. -->
**Tools used:** ChatGPT assisted with drafting this log; g++ and sanitizers were used to compile and check the code.

## 1. Hash table

The registry uses separate chaining, so each bucket holds a vector of owning pointers. A collision adds another entry to that bucket. I chose chaining because it makes collisions straightforward and permits a lookup to inspect just one bucket. Linear probing avoids a separate vector per bucket but requires careful handling, and its probe lengths depend strongly on the hash and load factor.

The hash is 64-bit FNV-1a. This combines each byte with the running value in order and returns the full hash. The bucket selection will reduce it by modulo the bucket count. The registry begins with 16 buckets. Before an insertion would make the load factor exceed 0.75, it doubles the bucket count and redistributes the entries using their hashes. This threshold trades extra bucket storage for shorter chains. During a resize the destination buckets reserve their required capacities before the owning pointers are moved.

For `tool_0` through `tool_9999`, the test measured 16,384 buckets, a load factor of 0.610352, and a longest chain of 6. At the same bucket count, the weak hash `s[0] + s.size()` produced a longest chain of 9,000. The names share their first character and most share their length, so that hash ignores nearly all the information that distinguishes them. These measurements are for this particular set of names, not a guarantee for arbitrary input.

An insertion normally examines one short chain. Rehashing takes time proportional to the number of stored entries, but doubling means that a large rehash happens only after many successful insertions. Spread over those insertions, the resize cost is constant per insertion; together with a well-distributed hash this gives expected amortized O(1) insertion. A poor distribution can still make a single chain long.

## 2. Pointer stability

Each bucket owns `unique_ptr<Tool>` objects, but the tools themselves live separately on the heap. Resizing moves the owning pointers into different bucket vectors; it does not move the objects they point to. Therefore a `Tool*` returned by `find` remains valid until the registry destroys that tool. A duplicate insertion destroys the incoming duplicate and leaves the original in place. The scale test retains the first pointer across all subsequent resizes and checks its identity again.

## 3. Calculator trace

The stacks below are listed bottom to top. Start with both empty. Reading `2` pushes it, giving values `[2]`. Reading `*` pushes that operator. Reading `(` puts it above `*`, and `3` gives values `[2, 3]`. The `+` goes above the opening parenthesis, because operators cannot cross it. Reading `4` gives `[2, 3, 4]`. On `)`, apply `+`: pop 4 and 3, push 7, then discard `(`. The stacks are values `[2, 7]` and operators `[*]`. When `-` arrives, `*` has higher precedence, so apply it and push 14; then push `-`. Reading `5` gives `[14, 5]`. At the end, apply `-` to obtain `[9]`. Equal precedence is also applied before pushing a new operator, preserving left-to-right evaluation.

## 4. Errors as values

A missing file, forbidden path, or zero divisor can be the result of a model's request. Returning a failure value lets the dispatcher record the error and the model decide what to do next without terminating the conversation. A message also gives useful context to the caller. Exceptions are more appropriate for programming mistakes or unexpected failures that cannot be handled as part of the normal tool configuration. The dispatcher provides a final exception boundary for unexpected failures.

## 5. What I would design differently

I would consider storing the registered name alongside its owning pointer. The current implementation calls `name()` again when searching and rehashing, which creates strings and assumes a tool's name stays constant. Storing the key would make that assumption explicit and reduce repeated name construction, at the cost of extra storage.

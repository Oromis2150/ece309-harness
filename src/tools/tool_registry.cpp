// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.
// Every function below is a stub so the starter compiles. Replace each body.

#include "tools/tool_registry.h"

ToolRegistry::ToolRegistry() : buckets_(16) {} 
ToolRegistry::~ToolRegistry() = default;   // unique_ptrs in your buckets free the tools

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    std::uint64_t h = UINT64_C(14695981039346656037);
    for (unsigned char c : key) { h ^= c; h *= UINT64_C(1099511628211);}
    return static_cast<std::size_t>(h);
}

void ToolRegistry::rehash(std::size_t count) {
    // Allocate destination storage before moving any owners.
    std::vector<std::vector<std::unique_ptr<Tool>>> next(count);
    std::vector<std::size_t> counts(count, 0);
    for (const auto& bucket : buckets_)
        for (const auto& tool : bucket) ++counts[hash(tool->name()) % count];
    for (std::size_t i = 0; i < count; ++i) next[i].reserve(counts[i]);
    for (auto& bucket : buckets_)
        for (auto& tool : bucket) {
            auto i = hash(tool->name()) % count;
            next[i].push_back(std::move(tool));
        }
    buckets_.swap(next);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) return false;
    const std::string name = tool->name();
    if (find(name)) return false;
    if (size_ + 1 > buckets_.size() * 3 / 4) rehash(buckets_.size() * 2);
    buckets_[hash(name) % buckets_.size()].push_back(std::move(tool));
    ++size_;
    return true;
}
Tool* ToolRegistry::find(std::string_view name) const {
    for (const auto& tool : buckets_[hash(name) % buckets_.size()])
        if (tool->name() == name) return tool.get();
    return nullptr;
}
std::size_t ToolRegistry::size() const noexcept { return size_; }
void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const auto& bucket : buckets_) for (const auto& tool : bucket) fn(*tool);
}
std::size_t ToolRegistry::bucket_count() const noexcept { return buckets_.size(); }
double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
}
std::size_t ToolRegistry::max_probe_length() const {
    std::size_t max = 0;
    for (const auto& bucket : buckets_) if (bucket.size() > max) max = bucket.size();
    return max;
}
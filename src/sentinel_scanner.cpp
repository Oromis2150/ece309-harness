#include "core/sentinel_scanner.h"
#include <utility>
#include <algorithm>

SentinelScanner::SentinelScanner(std::string sentinel) : sentinel_(std::move(sentinel)) {

}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    pending_.append(chunk);

    std::size_t safe_index = pending_.find(sentinel_);
    if (safe_index != std::string::npos) {
        std::string safe = pending_.substr(0, safe_index);
        pending_.clear();
        return {std::move(safe), true};
    }

    std::size_t keep = std::min(sentinel_.size() - 1, pending_.size());
    std::string safe = pending_.substr(0, pending_.size() - keep);
    pending_.erase(0, safe.size());
    return {safe, false};
}

SentinelScanner::Out SentinelScanner::flush() {
    Out out{ pending_, false };
    pending_.clear();
    return out;
}
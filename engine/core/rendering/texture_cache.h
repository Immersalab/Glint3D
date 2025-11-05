#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Texture.h"

// Simple global texture cache keyed by path + flip flag
class TextureCache {
public:
    static TextureCache& instance();
    Texture* get(const std::string& path, bool flipY);
    void clear();
private:
    struct Key { std::string path; bool flip; };
    struct KeyHash { size_t operator()(Key const& k) const; };
    struct KeyEq { bool operator()(Key const& a, Key const& b) const; };
    std::unordered_map<Key, std::unique_ptr<Texture>, KeyHash, KeyEq> m_cache;
};

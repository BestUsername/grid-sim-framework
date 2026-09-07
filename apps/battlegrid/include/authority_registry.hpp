#ifndef BATTLEGRID_AUTHORITY_REGISTRY_HPP_INCLUDED
#define BATTLEGRID_AUTHORITY_REGISTRY_HPP_INCLUDED

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace battlegrid {

/**
 * @brief Tracks the sole compute owner and ownership generation of each entity.
 *
 * Session pointers are opaque transport identities. They are never serialized;
 * the generation invalidates queued state from a previous owner after a
 * disconnect or reassignment.
 */
class AuthorityRegistry {
public:
    using Owner = const void*;

    /// Assign @p entityName to @p owner and advance its ownership generation.
    uint64_t assign(const std::string& entityName, Owner owner);

    /// True when @p owner is the entity's current remote owner.
    bool acceptsSnapshot(const std::string& entityName, Owner owner) const;

    /// Release every entity owned by @p owner and return the reclaimed names.
    std::vector<std::string> releaseOwner(Owner owner);

    /// True if the entity is currently assigned to any remote owner.
    bool isRemoteOwned(const std::string& entityName) const;

    /// Return the current ownership generation, or zero for an unknown entity.
    uint64_t epoch(const std::string& entityName) const;

private:
    struct Ownership {
        Owner owner = nullptr;
        uint64_t epoch = 0;
    };

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Ownership> m_ownership;
};

} // namespace battlegrid

#endif // BATTLEGRID_AUTHORITY_REGISTRY_HPP_INCLUDED

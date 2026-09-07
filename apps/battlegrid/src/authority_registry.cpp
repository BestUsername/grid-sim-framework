#include "authority_registry.hpp"

namespace battlegrid {

uint64_t AuthorityRegistry::assign(const std::string& entityName, Owner owner)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& ownership = m_ownership[entityName];
    ++ownership.epoch;
    ownership.owner = owner;
    return ownership.epoch;
}

bool AuthorityRegistry::acceptsSnapshot(const std::string& entityName, Owner owner) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_ownership.find(entityName);
    return it != m_ownership.end() && it->second.owner == owner;
}

std::vector<std::string> AuthorityRegistry::releaseOwner(Owner owner)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> reclaimed;
    for (auto& [entityName, ownership] : m_ownership) {
        if (ownership.owner == owner) {
            ownership.owner = nullptr;
            ++ownership.epoch;
            reclaimed.push_back(entityName);
        }
    }
    return reclaimed;
}

bool AuthorityRegistry::isRemoteOwned(const std::string& entityName) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_ownership.find(entityName);
    return it != m_ownership.end() && it->second.owner != nullptr;
}

uint64_t AuthorityRegistry::epoch(const std::string& entityName) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_ownership.find(entityName);
    return it != m_ownership.end() ? it->second.epoch : 0;
}

} // namespace battlegrid

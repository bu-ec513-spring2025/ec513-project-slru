
#include "mem/cache/replacement_policies/slru_rp.hh"

#include <cassert>
#include <limits>
#include <memory>

#include "params/SLRURP.hh"
#include "sim/cur_tick.hh"

namespace gem5 {
namespace replacement_policy {

SLRU::SLRU(const Params &p)
  : Base(p),
    protectedSize(p.protected_size),
    probationSize(p.probation_size),
    protectedEntries(0)
{}

void
SLRU::invalidate(const std::shared_ptr<ReplacementData>& rd)
{
    auto data = std::static_pointer_cast<SLRUReplData>(rd);
    // Remove from protected list if it was there
    if (data->segment == SLRUReplData::Protected) {
        auto it = std::find(protectedList.begin(), protectedList.end(), rd);
        if (it != protectedList.end()) {
            protectedList.erase(it);
            protectedEntries--;
        }
    }
    data->segment   = SLRUReplData::Probation;
    data->lastTouch = Tick(0);
}

void
SLRU::reset(const std::shared_ptr<ReplacementData>& rd) const
{
    auto data = std::static_pointer_cast<SLRUReplData>(rd);
    // Remove from protected list if it was there
    if (data->segment == SLRUReplData::Protected) {
        auto it = std::find(protectedList.begin(), protectedList.end(), rd);
        if (it != protectedList.end()) {
            protectedList.erase(it);
            protectedEntries--;
        }
    }
    data->segment   = SLRUReplData::Probation;
    data->lastTouch = curTick();
}

void
SLRU::touch(const std::shared_ptr<ReplacementData>& rd) const
{
    auto data = std::static_pointer_cast<SLRUReplData>(rd);
    Tick now = curTick();

    if (data->segment == SLRUReplData::Probation) {
        if (protectedEntries < protectedSize) {
            data->segment = SLRUReplData::Protected;
            protectedList.push_back(rd);
            protectedEntries++;
        } else {
            // Find LRU in protected segment by comparing lastTouch ticks
            auto lru_it = protectedList.begin();
            Tick oldest = std::static_pointer_cast<SLRUReplData>(*lru_it)->lastTouch;
            
            for (auto it = protectedList.begin(); it != protectedList.end(); ++it) {
                auto entry = std::static_pointer_cast<SLRUReplData>(*it);
                if (entry->lastTouch < oldest) {
                    oldest = entry->lastTouch;
                    lru_it = it;
                }
            }

            // Demote the LRU protected entry to probation
            auto lru_data = std::static_pointer_cast<SLRUReplData>(*lru_it);
            lru_data->segment = SLRUReplData::Probation;
            protectedList.erase(lru_it);
            
            // Promote current entry to protected
            data->segment = SLRUReplData::Protected;
            protectedList.push_back(rd);
            // protectedEntries remains the same (we replaced one)
        }
    } else {
        // If already in protected, no need to do
    }
    data->lastTouch = now;
}

ReplaceableEntry*
SLRU::getVictim(const ReplacementCandidates& candidates) const
{
    assert(!candidates.empty());

    ReplaceableEntry* oldestProb = nullptr;
    Tick minProb = std::numeric_limits<Tick>::max();

    for (auto *ent : candidates) {
        auto data = std::static_pointer_cast<SLRUReplData>(
            ent->replacementData);

        if (data->segment == SLRUReplData::Probation) {
            if (data->lastTouch < minProb) {
                minProb      = data->lastTouch;
                oldestProb   = ent;
            }
        }
    }
    // We must have at least one probationary block to evict
    assert(oldestProb && "No probationary entries available");
    return oldestProb;

}

std::shared_ptr<ReplacementData>
SLRU::instantiateEntry()
{
    return std::make_shared<SLRUReplData>();
}

}
}

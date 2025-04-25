
#pragma once

#include "params/SLRURP.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "sim/cur_tick.hh"
#include <memory>
#include <vector>

namespace gem5 {
namespace replacement_policy {

class SLRUReplData : public ReplacementData
{
  public:
    enum Segment : uint8_t { Probation = 0, Protected = 1 };

    Segment segment;
    Tick lastTouch;

    SLRUReplData()
      : segment(Probation),
        lastTouch(Tick(0))
    {}
};

class SLRU : public Base
{
  public:
    using Params = SLRURPParams;

    /**
     * @param p.protected_size 
     * @param p.probation_size
     */
    SLRU(const Params &p);
    ~SLRU() override = default;

    void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
    void reset    (const std::shared_ptr<ReplacementData>& rd) const override;
    void touch    (const std::shared_ptr<ReplacementData>& rd) const override;
    ReplaceableEntry* getVictim(
        const ReplacementCandidates& candidates) const override;

    std::shared_ptr<ReplacementData> instantiateEntry() override;

  private:
    const unsigned protectedSize;
    const unsigned probationSize;
    mutable unsigned protectedEntries;

    mutable std::vector<std::shared_ptr<ReplacementData>> protectedList;
};

}
}

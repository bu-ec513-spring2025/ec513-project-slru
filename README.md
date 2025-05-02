# Segmented LRU (SLRU) Replacement Policy for gem5

This repository implements a Segmented Least Recently Used (SLRU) cache replacement policy within the gem5 simulator. SLRU divides the cache entries into two segments:

* **Probationary Segment**: Newly accessed or cold entries.
* **Protected Segment**: Frequently accessed (hot) entries.

Entries promoted from probation to protected are subject to a capacity limit; when the protected segment is full, the least recently used block in the protected segment is demoted back to probation.

---

## Table of Contents

1. [Features](#features)
2. [Repository Structure](#repository-structure)
3. [Design and Data Structures](#design-and-data-structures)
4. [Integration with gem5](#integration-with-gem5)
5. [Behavior and Algorithms](#behavior-and-algorithms)
6. [Configuration Parameters](#configuration-parameters)

---

## Features

* **Two-Level Segmentation**: Separates cache entries into probationary and protected lists.
* **Adaptive Segment Adjustment**: On each access (`touch`), blocks move between segments according to recency and available capacity; a capped protected segment ensures the probationary list is never empty, so eviction candidates always exist.
* **Simple Victim Selection**: Always evict the least recently used entry from the probationary segment.
* **Configurable Sizes**: The maximum counts for each segment are set via constructor parameters.

---

## Repository Structure

```plaintext
src/
  mem/
    cache/
      replacement_policies/
        ReplacementPolicies.py    # Policy registration for Python config
        SConscript               # Build script including slru_rp.cc
        slru_rp.hh               # Header defining SLRUReplData and class interface
        slru_rp.cc               # Implementation of SLRU methods
    ruby/
      structures/
        RubyCache.py             # Change cache default policy to SLRU
```

---

## Design and Data Structures

### `SLRUReplData` (in `slru_rp.hh`)

```cpp
struct SLRUReplData : public ReplacementData {
    enum Segment : uint8_t { Probation = 0, Protected = 1 };
    Segment segment;      // Current segment of the entry
    Tick lastTouch;       // Timestamp of the last access

    SLRUReplData()
      : segment(Probation),
        lastTouch(Tick(0))
    {}
};
```

### `SLRU` Class (in `slru_rp.cc`)

```cpp
class SLRU : public Base {
  private:
    const unsigned protectedSize;
    const unsigned probationSize;
    mutable unsigned protectedEntries;
    mutable std::vector<std::shared_ptr<ReplacementData>> protectedList;

  public:
    using Params = SLRURPParams;

    SLRU(const Params &p);
    ~SLRU() override = default;

    void invalidate(const std::shared_ptr<ReplacementData>& rd) override;
    void reset    (const std::shared_ptr<ReplacementData>& rd) const override;
    void touch    (const std::shared_ptr<ReplacementData>& rd) const override;
    ReplaceableEntry* getVictim(
        const ReplacementCandidates& candidates) const override;
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};
```

---

## Integration with gem5

1. Place `ReplacementPolicies.py` and `SConscript` under `/src/mem/cache/replacement_policies/`.
2. Place `RubyCache.py` under `/src/mem/ruby/structures/`.
3. Place `slru_rp.hh` and `slru_rp.cc` under `/src/mem/cache/replacement_policies/`.

### Modifications to Existing Files

- **SConscript**: Added `slru_rp.cc` to the source list and appended `SLRURP` to the policy registry, ensuring the new code is built and linked with gem5.  
- **ReplacementPolicies.py**: Introduced the `SLRURP` class with `protected_size` and `probation_size` parameters for Python-based simulation configuration.  
- **RubyCache.py**: Changed the default `replacement_policy` to `SLRURP(protected_size, probation_size)` to allow immediate use of SLRU in Ruby cache models.  

These updates integrate the SLRU policy into both the gem5 build system and its Python/Ruby configuration layers, making it available for use in simulations.

---

## Behavior and Algorithms

### On `touch(const std::shared_ptr<ReplacementData>& rd) const`

* If `rd` is in Probationary:

  1. If `protectedEntries < protectedSize`, promote to Protected and add to `protectedList`.
  2. Otherwise, demote the LRU Protected entry to Probationary (remove from `protectedList`), then promote `rd`.
* If already Protected: no segment change.
* Always update `rd->lastTouch = curTick()`.

### On `invalidate` or `reset`

* If in Protected: remove `rd` from `protectedList` and decrement `protectedEntries`.
* Set `rd->segment = Probationary`.
* For **invalidate**: `rd->lastTouch = Tick(0)`.
* For **reset**: `rd->lastTouch = curTick()`.

### On `getVictim(const ReplacementCandidates& candidates) const`

* Scan all candidates with `segment == Probationary`.
* Return the entry with the smallest `lastTouch`.
* Assert that at least one Probationary entry exists.

---

## Configuration Parameters

| Parameter        | Description                                 |
| ---------------- | ------------------------------------------- |
| `protected_size` | Maximum entries in the protected segment    |
| `probation_size` | Maximum entries in the probationary segment |

---

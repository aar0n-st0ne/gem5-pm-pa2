#include "mem/cache/prefetch/cs395t_pf.hh"
#include "params/CS395TPrefetcher.hh"

#include <cassert>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"

// TODO FIXME: See stride.cc/.hh for reference.  There are many utility
// functions that may help with building a prefetcher.

namespace gem5
{

namespace prefetch
{

CS395TPrefetcherTables::CS395TPrefetcherTables(
   const CS395TPrefetcherTablesParams &p) : SimObject(p),
   deltaBits(p.delta_bits), deltaMaskBits(p.delta_mask_bits),
   table(p.table_assoc, p.table_entries, p.table_indexing_policy,
         p.table_replacement_policy, CS395TPrefetcherEntry(p.deltas_per_entry, p.nDelta)),
   nPC(p.nPC),
   lastPC(nPC, 0)
{
}

void
CS395TPrefetcherTables::CS395TPrefetcherEntry::invalidate()
{
    TaggedEntry::invalidate();

    deltas.flush();
    while (!deltas.full()) {
        deltas.push_back(0);
    }
    lastAddress = 0;
}

void
CS395TPrefetcherTables::CS395TPrefetcherEntry::addAddress(Addr address,
    unsigned int delta_bits)
{
    if ((address - lastAddress) != 0) {
        Addr delta = address - lastAddress;
        // Account for the sign bit
        Addr max_positive_delta = (1 << (delta_bits-1)) - 1;
        if (address > lastAddress) {
            // check positive delta overflow
            if (delta > max_positive_delta) {
                delta = 0;
            }
        } else {
            // check negative delta overflow
            if (lastAddress - address > (max_positive_delta + 1)) {
                delta = 0;
            }
        }
        deltas.push_back(delta);
        lastAddress = address;
    }
}

void
CS395TPrefetcherTables::CS395TPrefetcherEntry::getCandidates(
    std::vector<Queued::AddrPriority> &pfs, unsigned int mask) const
{
    assert(deltas.full());

    // Get the two most recent deltas
    const int delta_penultimate = *(deltas.end() - 2);
    const int delta_last = *(deltas.end() - 1);

    // a delta 0 means that it overflowed, we can not match it
    if (delta_last == 0 || delta_penultimate == 0) {
        return;
    }

    // Try to find the two most recent deltas in a previous position on the
    // delta circular array, if found, start issuing prefetches using the
    // remaining deltas (adding each delta to the last Addr to generate the
    // prefetched address.
    //printf("nDelta: %d\n", nDelta);
    for(int i=nDelta; i>=2; i--) {
        bool term = false;
        auto it = deltas.begin();
        for (; it != (deltas.end() - i); ++it) {
            //const int prev_delta_penultimate = *it;
            //const int prev_delta_last = *(it + 1);
            bool flag = true;
            for(int j = 0; j<i; j++) {
                if(((*(it+j))>>mask) != ((*(deltas.end()-i+j))>>mask))
                    flag = false;
            }
            if (flag) {
                // Pattern found. Skip the matching pair and issue prefetches with
                // the remaining deltas
                it += i;
                Addr addr = lastAddress;
                while (it != deltas.end()) {
                    const int pf_delta = *(it++);
                    addr += pf_delta;
                    pfs.push_back(Queued::AddrPriority(addr, 0));
                }
                term = true;
                break;
            }
        }
        if(term) break;
    }
}

void
CS395TPrefetcherTables::calculatePrefetch(
    const Base::PrefetchInfo &pfi,
    std::vector<Queued::AddrPriority> &addresses,
    const CacheAccessor &cache)
{
    if (!pfi.hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }
    Addr address = pfi.getAddr();

    // Shifting the last n PCs in the storage
    //printf("nPC: %d\n", nPC);
    for(int i=nPC-1; i>0; i--) {
        lastPC[i] = lastPC[i-1];
    }
    lastPC[0] = pfi.getPC();
    Addr pc = 0;
    // Hashing the last n PCs to get the index
    for(int i=0; i<nPC; i++) pc ^= lastPC[i];

    // Look up table entry, is_secure is unused in findEntry because we
    // index using the pc
    CS395TPrefetcherEntry *entry = table.findEntry(pc, false /* unused */);
    if (entry != nullptr) {
        entry->addAddress(address, deltaBits);
        //Delta correlating
        entry->getCandidates(addresses, deltaMaskBits);
    } else {
        entry = table.findVictim(pc);

        table.insertEntry(pc, false /* unused */, entry);

        entry->lastAddress = address;
    }
}

CS395TPrefetcher::CS395TPrefetcher(const CS395TPrefetcherParams &p)
  : Queued(p), dcpt(*p.dcpt), nPC(p.nPC), nDelta(p.nDelta)
{
}

void
CS395TPrefetcher::calculatePrefetch(const PrefetchInfo &pfi,
    std::vector<AddrPriority> &addresses,
    const CacheAccessor &cache)
{
    dcpt.calculatePrefetch(pfi, addresses, cache);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*CS395TPrefetcher::CS395TPrefetcher(const CS395TPrefetcherParams &params)
    : Queued(params), example_size(params.size)
    // TODO: Add more params here
{
}

void
CS395TPrefetcher::calculatePrefetch(const PrefetchInfo &pfi,
                                    std::vector<AddrPriority> &addresses,
                                    const CacheAccessor &cache)
{
    // TODO FIXME: Generate a prefetch.
}*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace prefetch
} // namespace gem5

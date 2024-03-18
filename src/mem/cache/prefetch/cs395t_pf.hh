#ifndef __MEM_CACHE_PREFETCH_CS395T_PF_HH__
#define __MEM_CACHE_PREFETCH_CS395T_PF_HH__
 
// #include <string>
// #include <unordered_map>
// #include <vector>
 
// #include "mem/cache/prefetch/queued.hh"
// #include "base/sat_counter.hh"
// #include "base/types.hh"
// #include "mem/cache/prefetch/associative_set.hh"
// #include "mem/cache/replacement_policies/replaceable_entry.hh"
// #include "mem/cache/tags/indexing_policies/set_associative.hh"
// #include "mem/packet.hh"
 
#include "base/circular_queue.hh"
#include "mem/cache/prefetch/associative_set.hh"
#include "mem/cache/prefetch/queued.hh"
 
namespace gem5
{
 
struct CS395TPrefetcherTablesParams;
struct CS395TPrefetcherParams;
 
namespace prefetch
{
class CS395TPrefetcherTables : public SimObject
{
    /** Number of bits of each delta */
    const unsigned int deltaBits;
    /** Number of lower bits to ignore from the deltas */
    const unsigned int deltaMaskBits;
 
    /** Storing last n PCs*/
    const unsigned int nPC;
    std::vector<Addr> lastPC;

    /** DCPT Table entry datatype */
    struct CS395TPrefetcherEntry : public TaggedEntry
    {
        /** Last accessed address */
        Addr lastAddress;
        /** Stored deltas */
        CircularQueue<Addr> deltas;
        
        unsigned int nDelta;
 
        /**
         * Constructor
         * @param num_deltas number of deltas stored in the entry
         */
        CS395TPrefetcherEntry(unsigned int num_deltas, unsigned int nDelta)
          : TaggedEntry(), lastAddress(0), deltas(num_deltas), nDelta(nDelta)
        {
        }
 
        void invalidate() override;
 
        /**
         * Adds an address to the entry, if the entry already existed, a delta
         * will be generated
         * @param address address to add
         * @param delta_num_bits number of bits of the delta
         */
        void addAddress(Addr address, unsigned int delta_num_bits);
 
        /**
         * Attempt to generate prefetch candidates using the two most recent
         * deltas. Prefetch candidates are added to the provided vector.
         * @param pfs reference to a vector where candidates will be added
         * @param mask_bits the number of lower bits that should be masked
         *        (ignored) when comparing deltas
         */
        void getCandidates(std::vector<Queued::AddrPriority> &pfs,
                           unsigned int mask_bits) const;
 
    };
    /** The main table */
    AssociativeSet<CS395TPrefetcherEntry> table;
 
  public:
    CS395TPrefetcherTables(
        const CS395TPrefetcherTablesParams &p);
    ~CS395TPrefetcherTables() = default;
 
    /**
     * Computes the prefetch candidates given a prefetch event.
     * @param pfi The prefetch event information
     * @param addresses prefetch candidates generated
     * @param cache accessor for cache lookups
     */
    void calculatePrefetch(const Base::PrefetchInfo &pfi,
        std::vector<Queued::AddrPriority> &addresses,
        const CacheAccessor &cache);
 
};
 
class CS395TPrefetcher : public Queued
{
   /** DCPT object */
  CS395TPrefetcherTables &dcpt;
  const unsigned int nPC;
  const unsigned int nDelta;
  public:
    CS395TPrefetcher(const CS395TPrefetcherParams &p);
    ~CS395TPrefetcher() = default;
 
    void calculatePrefetch(const PrefetchInfo &pfi,
        std::vector<AddrPriority> &addresses,
        const CacheAccessor &cache) override;
  //protected:
    //const unsigned example_size; // example parameter
    // TODO: Add more params here
};
 
} // namespace prefetch
} // namespace gem5
 
#endif // __MEM_CACHE_PREFETCH_CS395T_PF_HH__
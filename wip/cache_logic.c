/* Only cache images, videos, html, css and js

Add file to cache for 15 new requests per hour (allowed 3 indpendent requests)
Maximum cache element size 50MB
Maximum total cache 2GB

Client side cache invalidation using a hashmap of md5 hashes

Create list of pointers to cache entries.
Reorder cache periodically (how often?) by number of hits
 TODO: if cache is full and new entry is required remove tail of list and add new cache entry.  */

#include <stdlib.h>
#include "cache.h"

// Comparison function that weights hits more than accessed time (in unix time)
int compare_cache_entries(const void *a, const void *b) {
    struct cache_entry *entryA = *(struct cache_entry**)a;
    struct cache_entry *entryB = *(struct cache_entry**)b;

    // Combine last_accessed and hits into a single value, giving more weight to hits
    // 16 is used as 3600/15 = 16*15
    long combinedValueA = entryA->last_accessed + 16 * entryA->hits;
    long combinedValueB = entryB->last_accessed + 16 * entryB->hits;

    return combinedValueA - combinedValueB;
}

// Sort list to pointers
void sort_cache_entries(struct cache_entry **entries, size_t num_entries) {
    qsort(entries, num_entries, sizeof(struct cache_entry*), compare_cache_entries);
}

// Reset hits in cache every hour
void reset_hit_counts(struct cache *cache) {
    struct cache_entry *current = cache->head;
    while (current != NULL) {
        current->hits = 0; // Reset hits to 0
        current = current->next;
    }
}

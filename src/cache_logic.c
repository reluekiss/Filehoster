/* Only cache images, videos, html, css and js

Add file to cache for 15 new requests per hour (allowed 3 indpendent requests)
Maximum cache element size 50MB
Maximum total cache 2GB

Client side cache invalidation using a hashmap of md5 hashes

Create list of pointers to cache entries.
Reorder cache periodically (how often?) by number of hits */
#include <stdlib.h>
#include <openssl/evp.h>
#include "cache.h"

// TODO: linking errors at compile time
// MD5 hashing by devtty1er https://stackoverflow.com/questions/7627723/how-to-create-a-md5-hash-of-a-string-in-c
void bytes2md5(const char *data, int len, char *md5buf) {
  // Based on https://www.openssl.org/docs/manmaster/man3/EVP_DigestUpdate.html
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  const EVP_MD *md = EVP_md5();
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len, i;
  EVP_DigestInit_ex(mdctx, md, NULL);
  EVP_DigestUpdate(mdctx, data, len);
  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_free(mdctx);
  for (i = 0; i < md_len; i++) {
    snprintf(&(md5buf[i * 2]), 16 * 2, "%02x", md_value[i]);
  }
}


// Find ce with lowest hit count
/* struct cache_entry sort_cache_entries(struct cache *cache) {
    int size = cache->cur_size;
    struct cache_entry *current = cache->head;
    int lowest = 0;
    for (int i = 0; i <= size; i++){
       if(current->hits = lowest)
            lowest = current->hits;
        current = current->next;
    }
} */

// Two pointer approach
struct cache_entry* sort_cache_entries(struct cache *cache) {
    int size = cache->cur_size;
    struct cache_entry *slow = cache->head;
    struct cache_entry *fast = cache->head;
    int lowest = 0;

    // Move slow one step at a time while fast moves two steps at a time
    // When fast reaches the end, slow will be at the middle
    while (fast != NULL && fast->next != NULL) {
        fast = fast->next->next;
        if (lowest < fast->hits) {
            lowest = fast->hits;
        }
        slow = slow->next;
    }

    // If there's an odd number of elements, move slow one more step
    if (fast != NULL) {
        slow = slow->next;
    }
    return slow;
}

// Reset hits in cache
void reset_hit_counts(struct cache *cache) {
    struct cache_entry *current = cache->head;
    while (current != NULL) {
        current->hits = 0; // Reset hits to 0
        current = current->next;
    }
}

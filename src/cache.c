#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "hashtable.h"
#include "cache.h"

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
struct cache_entry* lowest_hit_entry(struct cache *cache) {
    int size = cache->cur_size;
    struct cache_entry *slow_ptr = cache->head, *fast_ptr = cache->head;
    int lowest = INT_MAX;

    while (fast_ptr != NULL && fast_ptr->next != NULL) {
        fast_ptr = fast_ptr->next->next;
        slow_ptr = slow_ptr->next;

        if (fast_ptr == NULL || fast_ptr->next == NULL)
            break;

        if (slow_ptr->hits < lowest) {
            lowest = slow_ptr->hits;
        } else if (slow_ptr->hits == lowest && slow_ptr->hits > 0) { 
            lowest = slow_ptr->hits;
        }
    }
    return slow_ptr;
}

// Reset hits in cache
void reset_hit_counts(struct cache *cache) {
    struct cache_entry *current = cache->head;
    while (current != NULL) {
        current->hits = 0; // Reset hits to 0
        current = current->next;
    }
}

/**
 * Allocate a cache entry
 */
struct cache_entry *alloc_entry(char *path, char *content_type, void *content, int content_length) {
    struct cache_entry *ce = malloc(sizeof *ce);

    if (ce == NULL) return NULL;

    ce->path = path;
    ce->content_type = content_type;
    ce->content = content;
    ce->content_length = content_length;

    char md5[33];
    bytes2md5(content, sizeof(content), md5);
    ce->md5 = md5;
    ce->hits = 0;
    
    ce->next = ce->prev = NULL;

    return ce;
}

/**
 * Insert a cache entry at the head of the linked list
 */
void dllist_insert_head(struct cache *cache, struct cache_entry *ce) {
    // Insert at the head of the list
    if (cache->head == NULL) {
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    } else {
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Removes an from the list and returns it
 *
 */
void dllist_remove_entry(struct cache *cache, struct cache_entry *ce) {
    ce->next->prev = ce->prev;
    ce->prev->next = ce->next;

    cache->cur_size--;

    free(ce);
}

/**
 * Create a new cache
 * 
 * max_size: maximum number of entries in the cache
 * hashsize: hashtable size (0 for default)
 */
struct cache *cache_create(int max_size, int hashsize) {
    struct cache *cache = malloc(sizeof *cache);

    cache->max_size = max_size;
    cache->cur_size = 0;
    cache->index = hashtable_create(hashsize, NULL);
    cache->head = cache->tail = NULL;
    
    return cache;
}

void cache_free(struct cache *cache) {
    struct cache_entry *cur_entry = cache->head;

    hashtable_destroy(cache->index);

    while (cur_entry != NULL) {
        struct cache_entry *next_entry = cur_entry->next;

        free(cur_entry);

        cur_entry = next_entry;
    }

    free(cache);
}

/**
 * Store an entry in the cache
 *
 * This will also remove the least-recently-used items as necessary.
 * 
 * NOTE: doesn't check for duplicate cache entries
 */
void cache_put(struct cache *cache, char *path, char *content_type, void *content, int content_length) {
    struct cache_entry *ce = alloc_entry(path, content_type, content, content_length);
    
    if(cache->cur_size >= cache->max_size)
        dllist_remove_entry(cache, lowest_hit_entry(cache));
    dllist_insert_head(cache, ce);
    hashtable_put(cache->index, path, ce);
    
    cache->cur_size++; 
}

/**
 * Retrieve an entry from the cache
 */
struct cache_entry *cache_get(struct cache *cache, char *path) {
    struct cache_entry *ce = hashtable_get(cache->index, path);
    ce->hits++;
    return ce;
}

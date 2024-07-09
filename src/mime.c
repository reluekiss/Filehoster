#include <string.h>
#include <ctype.h>
#include "hashtable.h"

#define DEFAULT_MIME_TYPE "application/octet-stream"

/*
 * Lowercase a string
 */
char *strlower(char *s) {
    for (char *p = s; *p != '\0'; p++) {
        *p = tolower(*p);
    }
    return s;
}

// Define the hash function for strings
int str_hash(void *data, int data_size, int bucket_count) {
    unsigned long hash = 5381;
    const char *s = data;
    while (*s) {
        hash = ((hash << 5) + hash) + tolower(*s++);
    }
    return hash % bucket_count;
}

/*
 * MIME type definition
 */
struct mime_type_hash {
    char *ext;
    char *mime_type;
};

/*
 * Initialize the hash table with the MIME types, only needs to be called once at program beginning
 * then kernel will remove any dangling pointers or memory when parent process dies
 */
struct hashtable* mime_type_hash_init() {
    struct mime_type_hash mime_types[] = {
        { "html", "text/html" },
        { "htm", "text/html" },
        { "jpeg", "image/jpeg" },
        { "jpg", "image/jpeg" },
        { "css", "text/css" },
        { "js", "application/javascript" },
        { "json", "application/json" },
        { "txt", "text/plain" },
        { "gif", "image/gif" },
        { "png", "image/png" },
    };
    
    struct hashtable *mime_types_ht = hashtable_create(10, str_hash);
    for(int i = 0; i < 10; i++) {
        hashtable_put(mime_types_ht, mime_types[i].ext, mime_types[i].mime_type);
    }
    return mime_types_ht;
}

/*
 * Return a MIME type for a given filename
 */
char* mime_type_get(char *filename, struct hashtable *ht) {
    char* ext = strrchr(filename, '.');

    if (ext == NULL) {
        return DEFAULT_MIME_TYPE;
    }
    ext++;
    strlower(ext);
    
    char* mime_type = hashtable_get(ht, ext);
    return mime_type;
}

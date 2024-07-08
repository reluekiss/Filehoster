#include <string.h>
#include <ctype.h>
#include "hashtable.h"
// TODO: I know that this is broken
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
void mime_type_hash_init() {
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
    hashtable_put(mime_types_ht, mime_type_hash, mime_types);
}

/*
 * Return a MIME type for a given filename
 */
char *mime_type_get(char *filename) {
    char *ext = strrchr(filename, '.');

    if (ext == NULL) {
        return DEFAULT_MIME_TYPE;
    }
    ext++;
    strlower(ext);
    
    char* mime_type = hashtable_get_bin(mime_types_ht, ext, strlen(ext));
    return mime_type;
}


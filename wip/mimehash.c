#include <string.h>
#include <ctype.h>
#include "uthash.h"

#define DEFAULT_MIME_TYPE "application/octet-stream"
/** * Lowercase a string
 */
char *strlower(char *s) {
    for (char *p = s; *p != '\0'; p++) {
        *p = tolower(*p);
    }
    return s;
}

/** * MIME type definition
 */
typedef struct {
    char *ext;
    char *mime_type;
    UT_hash_handle hh;
} mime_type_hash;

/** * Return a MIME type for a given filename
 */
char *mime_type_get(char *filename)
{
    char *ext = strrchr(filename, '.');

    if (ext == NULL) {
        return DEFAULT_MIME_TYPE;
    }

    ext++;

    strlower(ext);
    mime_type_hash *mime_type_hash = NULL;
    HASH_FIND_STR(mime_type_hash, ext, mime_type_hash);

    if (mime_type_hash == NULL) {
        return DEFAULT_MIME_TYPE;
    }

    return mime_type_hash->mime_type;
}

/** * Initialize the hash table with the MIME types
 */
void mime_type_hash_init()
{
    mime_type_hash mime_types[] = {
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

    for (int i = 0; i < sizeof(mime_types) / sizeof(mime_types[0]); i++) {
        mime_type_hash *mime_type_hash_entry = malloc(sizeof(mime_type_hash));
        mime_type_hash_entry->mime_type = malloc(sizeof(mime_type_hash));
        mime_type_hash_entry->ext = strdup(mime_types[i].ext);
        mime_type_hash_entry->mime_type = strdup(mime_types[i].mime_type);
        HASH_ADD_STR(mime_type_hash, ext, mime_type_hash_entry);
    }
}

/** * Clean up the hash table
 */
void mime_type_hash_cleanup()
{
    mime_type_hash *current_mime_type_hash, *tmp_mime_type_hash;

    HASH_ITER(hh, mime_type_hash, current_mime_type_hash, tmp_mime_type_hash) {
        HASH_DEL(mime_type_hash, current_mime_type_hash);
        free(current_mime_type_hash->ext);
        free(current_mime_type_hash->mime_type);
        free(current_mime_type_hash);
    }
}

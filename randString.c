#include <stdio.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+
+ Binary Search
+
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int binarySearch(char* arr[], int l, int r, char* x) {
    if(r >= l) {
        int mid = l + (r-l)/2;

        if(arr[mid] == x)
            return 1;

        if(arr[mid] < x)
            return binarySearch(arr, mid+1, r, x);

        return binarySearch(arr, l, mid-1, x);
    }
    return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+
+ Open all files from a directory as an array.
+
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char** fileList(char *dir) {
    struct dirent *ent;
    char** filenames = malloc(4096 * sizeof(char*));
    for(int i = 0; i < 4096; i++) {
        filenames[i] = malloc(16 * sizeof(char));
    }
    int i = 0;
    DIR *Dir;

    Dir = opendir(dir); // Replace with your directory path
    if (dir!= NULL) {
        while ((ent = readdir(Dir))!= NULL) {
            strncpy(filenames[i], ent->d_name, 15);
            filenames[i][strlen(ent->d_name)] = '\0'; // Null-terminate the string
            i++;
            if(i >= 4096) break; // Prevent buffer overflow
        }
        closedir(Dir);
    } else {
        perror("Could not open directory");
    }
    return filenames;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +
 + Generate a random string for a file using urandom.
 +      - Don't forget to free() the generated string.
 +
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char* randString(char* webdir, char *s, int len) {
    const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int i, n;

    srand(time(NULL));
    for (i = 0; i < len; i++) {
        n = rand() % (int)(strlen(chars) - 1);
        s[i] = chars[n];
    }
    s[len] = '\0';

    // Check if filename exists.
    char** filenames = fileList(webdir);
    if (binarySearch(filenames, 0, 4096, s) == 1) {
        for(int i = 0; i < 4096; i++) {
            free(filenames[i]);
        }
        free(filenames);
        return randString(webdir, s, len);
    }
    
    for(int i = 0; i < 4096; i++) {
        free(filenames[i]);
    }
    free(filenames);
    return s; 
}

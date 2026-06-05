#include "path-handler.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "static-response-bodies/default_dir_list_page.h"

static int dir_filter(const struct dirent *entry);


int list_dir(char *path, char *buffer) {
    struct dirent **entries;
    int n;
    int size = 0;

    n = scandir(path, &entries, dir_filter, NULL);

    if (n < 0) {
        perror("scandir");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        if (entries[i]->d_type == DT_DIR) {

            char dir[strlen(entries[i]->d_name) + 1];
            sprintf(dir, "%s/",entries[i]->d_name);

            size += snprintf(buffer + size, 256, ELEMENT_FORMAT,
                dir, dir, "directory");

        } else {
            size += snprintf(buffer + size, 256, ELEMENT_FORMAT,
                entries[i]->d_name, entries[i]->d_name, "file");
        }
        free(entries[i]);
    }

    free(entries);
    return 0;
}

static int dir_filter(const struct dirent *entry) {
    return strcmp(entry->d_name, ".") != 0 &&
        strcmp(entry->d_name, "..") != 0;
}

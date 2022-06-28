#ifndef JSON_C
#define JSON_C
#include <json.h>
#endif //JSON_C
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/anime_functions.h"

#define XDG_CONFIG_HOME_DEFAULT "~/.config"
#define APP_SUBFOLDER "/aweek-c"
#define SAVED_ANIME_FILENAME "/anime.json"

char* get_save_anime_filepath() {
    char * config_filepath = getenv("XDG_CONFIG_HOME");
    if (config_filepath == NULL) {
        config_filepath = XDG_CONFIG_HOME_DEFAULT;
    }
    size_t config_filepath_len = sizeof(char) * strlen(config_filepath);
    size_t app_subfolder_len = sizeof(char) * strlen(APP_SUBFOLDER);
    size_t saved_anime_filename_len = sizeof(char) * strlen(SAVED_ANIME_FILENAME);
    size_t written = 0;
    struct stat sb;

    char * filepath = malloc(config_filepath_len + app_subfolder_len + saved_anime_filename_len + 1);
    if (filepath == NULL)  return NULL;

    memcpy(filepath, config_filepath, config_filepath_len);
    written += config_filepath_len;
    filepath[written] = '\0';
    if (stat(filepath, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "XDG_CONFIG_HOME does not exist!\n");
        free(filepath);
        return NULL;
    }

    memcpy(filepath+written, APP_SUBFOLDER, app_subfolder_len);
    written += app_subfolder_len;
    filepath[written] = '\0';
    if (stat(filepath, &sb) !=0 || !S_ISDIR(sb.st_mode)) {
        // creating app subfolder in XDG_CONFIG_HOME
        if (mkdir(filepath, 0755) == -1) {
            fprintf(stderr, "Failed to create app subfolder\n");
            free(filepath);
            return NULL;
        }
    }

    memcpy(filepath+written, SAVED_ANIME_FILENAME, saved_anime_filename_len);
    written += saved_anime_filename_len;
    filepath[written] = '\0';

    if (access(filepath, F_OK|R_OK|W_OK) != 0 && access(filepath, F_OK) == 0) {
        fprintf(stderr, "Anime file is not readable or writable\n");
        free(filepath);
        return NULL;
    }

    return filepath;
}

struct json_object * load_saved_anime(char* filepath) {
    struct stat sb;
    if (stat(filepath, &sb) != 0) {
        fprintf(stderr, "Failed to get file information\n");
    }

    char * buffer = malloc(sb.st_size);
    if (buffer == NULL) return NULL;

    FILE * file = fopen(filepath, "r");
    if (fread(buffer, sb.st_size, 1, file) != 1) {
        fprintf(stderr, "Failed to read file\n");
        fclose(file);
        free(buffer);
        return NULL;
    }
    fclose(file);

    struct json_object * anime_array;
    anime_array = json_tokener_parse(buffer);
    free(buffer);

    if (!json_object_is_type(anime_array, json_type_array)) {
        fprintf(stderr, "Json object is not an array\n");
        free(anime_array);
        return NULL;
    }
    printf("Anime array len: %zu\n", json_object_array_length(anime_array));

    return anime_array;
}

int main() {
    char * filepath = get_save_anime_filepath();
    if (filepath == NULL) return -1;

    struct json_object * anime_array;
    anime_array = load_saved_anime(filepath);
    if (anime_array == NULL) {
        free(filepath);
        return -2;
    }

    list_all(anime_array);

    free(anime_array);
    free(filepath);
    return 0;
}

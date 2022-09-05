#include <json.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/anime_functions.h"

#define XDG_CONFIG_HOME_DEFAULT "~/.config"
#define APP_SUBFOLDER "/aweek"
#define SAVED_ANIME_FILENAME "/anime.json"

#define APP_NAME "aweek"
#define VERSION "1.0.0{GIT-COMMIT}"

/**
 * Print program version
 * @return always 0
 */
int print_version() {
    fputc('\n', stdout);
    fprintf(stdout, APP_NAME " " VERSION "\n");
    fputc('\n', stdout);
    return 0;
}

/**
 * Print help
 * @return always 0
 */
int print_help() {
    print_version();
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "\t" APP_NAME "                                                  list new episodes\n");
    fprintf(stdout, "\t" APP_NAME " (a)dd                                            add anime\n");
    fprintf(stdout, "\t" APP_NAME " (d)elete     <anime_id>                          delete anime\n");
    fprintf(stdout, "\t" APP_NAME " (u)pdate     <anime_id> <downloaded_episodes>    update anime's downloaded episodes count\n");
    fprintf(stdout, "\t" APP_NAME " (e)dit       <anime_id>                          edit anime\n");
    fprintf(stdout, "\t" APP_NAME " (i)gnore     <anime_id>                          toggle ignored flag for anime\n");
    fprintf(stdout, "\t" APP_NAME " (l)ist                                           list all anime\n");
    fprintf(stdout, "\t" APP_NAME " (n)ew-episodes-count                             show the number of new episodes\n");
    fprintf(stdout, "\t" APP_NAME " (v)ersion                                        print version information\n");
    fprintf(stdout, "\t" APP_NAME " anything else                                    print this help page\n");
    return 0;
}

/**
 * Get filepath to the file used to save anime array
 * Creates folders if necessary, respects XDG Base Directory
 * @return filepath to use when saving anime array, or NULL on error
 */
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

/**
 * Load anime array from json file
 * @param filepath file to load anime array from
 * @return pointer to json object representing anime array, or NULL on error
 */
struct json_object * load_saved_anime(char* filepath) {
    struct stat sb;
    if (stat(filepath, &sb) != 0) { // file does not exist
        return json_object_new_array_ext(1);
    }

    char * buffer = malloc(sb.st_size + 1);
    if (buffer == NULL) return NULL;
    buffer[sb.st_size] = '\0';

    FILE * file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open the file for reading\n");
        free(buffer);
        return NULL;
    }
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

    return anime_array;
}

/**
 * Save anime array as a json file
 * @param filepath file to save anime array to
 * @param anime_array anime array to save
 * @return 0 on success, otherwise -1 on error
 */
int save_anime(char* filepath, struct json_object * anime_array) {
    const char * json_str;
    size_t json_str_len;
    FILE * file = fopen(filepath, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open the file for writing\n");
        return -1;
    }

    json_str = json_object_to_json_string_ext(anime_array, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED);
    json_str_len = strlen(json_str);

    if (fwrite(json_str, sizeof(char), json_str_len, file) != json_str_len) {
        fprintf(stderr, "Failed to write anime information into the file\n");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

/**
 * Process arguments and take an appropriate action
 * @param argc number of arguments
 * @param argv arguments array
 * @param anime_array anime array to use in actions
 * @return on success, 1 is returned if saving is necessary, 0 if not, otherwise -1 on error
 */
int process_args_do_action(int argc, char ** argv, struct json_object * anime_array) {
    if (argc == 1) {
        print_new_episodes(anime_array);
        return 0;
    }

    size_t anime_id = 0, episodes = 0;
    if (argc > 2) {
        anime_id = strtoul(argv[2], NULL, 10) - 1;
        if (anime_id >= json_object_array_length(anime_array)) {
            fprintf(stderr, "No anime with id %zu.\n", anime_id + 1);
            return -1;
        }
    }
    if (argc > 3) {
        episodes = strtoul(argv[3], NULL, 10);
    }

    if ('a' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("add", argv[1]) == 0)) { // ADD
        return add_anime(anime_array, MANUAL) == 0 ? 1 : -1;
    } else if ('d' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("delete", argv[1]) == 0)) { // DELETE
        if (argc < 3) {
            fprintf(stderr, "Please specify id of the anime to delete.\n");
            return -1;
        }
        return delete_anime(anime_array, anime_id) == 0 ? 1 : -1;
    } else if ('u' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("update", argv[1]) == 0)) { // UPDATE
        if (argc < 3) {
            fprintf(stderr, "Please specify id of the anime to update and new downloaded episodes count.\n");
            return -1;
        }
	if (argc == 3) {
	    // Quick update
	    return update_anime_quick(json_object_array_get_idx(anime_array, anime_id)) == 0 ? 1 : -1;
	} else {
	    // Regular update
            return update_anime(json_object_array_get_idx(anime_array, anime_id), episodes) == 0 ? 1 : -1;
	}
    } else if ('e' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("edit", argv[1]) == 0)) { // EDIT
        if (argc < 3) {
            fprintf(stderr, "Please specify id of the anime to edit.\n");
            return -1;
        }
        return edit_anime(json_object_array_get_idx(anime_array, anime_id)) == 0 ? 1 : -1;
    } else if ('i' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("ignore", argv[1]) == 0)) { // IGNORE
        if (argc < 3) {
            fprintf(stderr, "Please specify id of the anime to toggle the ignore flag on.\n");
            return -1;
        }
        return toggle_anime_ignored(json_object_array_get_idx(anime_array, anime_id)) == 0 ? 1 : -1;
    } else if ('l' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("list", argv[1]) == 0)) { // LIST
        return list_all(anime_array);
    } else if ('n' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("new-episodes-count", argv[1]) == 0)) { // NEW EPISODES COUNT
        return print_new_episodes_count(anime_array);
    } else if ('v' == argv[1][0] && (strlen(argv[1]) == 1 ||  strcmp("version", argv[1]) == 0)) { // VERSION
        return print_version();
    } else { // HELP
        return print_help();
    }

    return -1;
}

/**
 * Main function
 * @param argc number of arguments
 * @param argv array of arguments
 * @return 0 on success, otherwise -1 on error
 */
int main(int argc, char ** argv) {
    char * filepath = get_save_anime_filepath();
    if (filepath == NULL) return -1;

    struct json_object * anime_array;
    anime_array = load_saved_anime(filepath);
    if (anime_array == NULL) {
        free(filepath);
        return -1;
    }

    int return_code = process_args_do_action(argc, argv, anime_array);

    if (return_code == 1) {
        save_anime(filepath, anime_array);
        return_code = 0;
    }

    json_object_put(anime_array);
    free(filepath);
    return return_code;
}

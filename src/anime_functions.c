// for strptime
#define _GNU_SOURCE
#include <json.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "../include/anime_functions.h"

/**
 * List all saved anime
 * @param anime_array json_object, must be of type json_type_array
 * @return -1 on error, otherwise 0
 */
int list_all(struct json_object * anime_array) {
    size_t i, n_anime;
    struct json_object * anime;
    struct json_object * anime_name;
    struct json_object * anime_episodes;
    struct json_object * anime_episodes_downloaded;
    struct json_object * anime_start_date;
    time_t start_unix;
    struct tm * start_datetime;
    char start_string[16];

    printf("%3c | %-30.30s | %-8.8s | %-22.22s\n", '#', "Anime name", "Episodes", "Broadcast (Local Time)");
    for (i=0; i<73; i++) putchar('-');
    putchar('\n');

    n_anime = json_object_array_length(anime_array);

    for (i=0; i<n_anime; i++) {
        anime = json_object_array_get_idx(anime_array, i);
        if (!json_object_object_get_ex(anime, "name", &anime_name)) {
            fprintf(stderr, "Malformed json\n");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "episodes", &anime_episodes)) {
            fprintf(stderr, "Malformed json\n");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "episodes_downloaded", &anime_episodes_downloaded)) {
            fprintf(stderr, "Malformed json\n");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "start_date", &anime_start_date)) {
            fprintf(stderr, "Malformed json\n");
            return -1;
        }
        start_unix = json_object_get_int64(anime_start_date);
        start_datetime = localtime(&start_unix);
        if (strftime(start_string, sizeof(start_string), "%A\t%H:%M", start_datetime) == 0) {
            fprintf(stderr, "Failed to fit formatted start date in a char array\n");
            return -1;
        }
        printf("%3zu | %-30.30s | %3d/%-4d | %-15.15s\n",
               i+1,
               json_object_get_string(anime_name),
               json_object_get_int(anime_episodes_downloaded),
               json_object_get_int(anime_episodes),
               start_string);
    }

    for (i=0; i<73; i++) putchar('-');
    putchar('\n');
    return 0;
}

/**
 * Helper function to get the number of available episodes for an anime
 * @param anime_array json_object, must be of type json_type_array
 * @param anime_at index of the anime to get the count for
 * @return the number of new episodes for the anime or -1 on error
 */
int get_new_episodes_count(struct json_object * anime_array, size_t anime_at) {
    size_t j, episodes_all, episodes_downloaded, episodes_available, n_episodes_delayed;
    struct json_object * anime;
    struct json_object * anime_episodes;
    struct json_object * anime_episodes_downloaded;
    struct json_object * anime_start_date;
    struct json_object * anime_delayed_episodes;
    struct json_object * delayed_episode;
    struct json_object * anime_ignored;
    time_t start_unix;
    time_t now = time(NULL);

    anime = json_object_array_get_idx(anime_array, anime_at);
    if (!json_object_object_get_ex(anime, "ignored", &anime_ignored)) {
        fprintf(stderr, "Malformed json\n");
        return -1;
    }
    // skip ignored
    if (json_object_get_boolean(anime_ignored)) return 0;

    if (!json_object_object_get_ex(anime, "episodes", &anime_episodes)) {
        fprintf(stderr, "Malformed json\n");
        return -1;
    }
    if (!json_object_object_get_ex(anime, "episodes_downloaded", &anime_episodes_downloaded)) {
        fprintf(stderr, "Malformed json\n");
        return -1;
    }
    episodes_all = json_object_get_int(anime_episodes);
    episodes_downloaded = json_object_get_int(anime_episodes_downloaded);
    // skip fully downloaded
    if (episodes_all <= episodes_downloaded) return 0;

    if (!json_object_object_get_ex(anime, "start_date", &anime_start_date)) {
        fprintf(stderr, "Malformed json\n");
        return -1;
    }
    start_unix = json_object_get_int64(anime_start_date);
    if (now < start_unix) return 0; // the anime hasn't started airing yet

    if (!json_object_object_get_ex(anime, "delayed_episodes", &anime_delayed_episodes)) {
        fprintf(stderr, "Malformed json\n");
        return -1;
    }
    n_episodes_delayed = json_object_array_length(anime_delayed_episodes);

    // calculating already aired episodes
    now -= start_unix;
    // count how many weeks have passed since start date, adding 1 because start date == first episode
    episodes_available = (now / (7 * 24 * 60 * 60)) + 1;
    for (j=0; j<n_episodes_delayed; j++) {
        delayed_episode = json_object_array_get_idx(anime_delayed_episodes, anime_at);
        if (json_object_get_uint64(delayed_episode) <= episodes_available) episodes_available--;
    }

    if (episodes_available <= episodes_downloaded) return 0;

    return (int) (episodes_available - episodes_downloaded);
}

/**
 * Print new episodes information
 * @param anime_array json_object, must be of type json_type_array
 * @return -1 on error, otherwise 0
 */
int print_new_episodes(struct json_object * anime_array) {
    int printed_something = 0;
    size_t i, j, n_anime, episodes_downloaded, episodes_available;
    struct json_object * anime;
    struct json_object * anime_name;
    struct json_object * anime_episodes_downloaded;

    n_anime = json_object_array_length(anime_array);

    for (i=0; i<n_anime; i++) {
        episodes_available = get_new_episodes_count(anime_array, i);

        anime = json_object_array_get_idx(anime_array, i);

        if (!json_object_object_get_ex(anime, "name", &anime_name)) {
            fprintf(stderr, "Malformed json\n");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "episodes_downloaded", &anime_episodes_downloaded)) {
            fprintf(stderr, "Malformed json\n");
            return -1;
        }
        episodes_downloaded = json_object_get_int(anime_episodes_downloaded);

        // printing out new episodes if any
        for (j=0; j<episodes_available; j++) {
            printf("NEW (%zu) \"%s\" episode #%zu\n",
                   i+1,
                   json_object_get_string(anime_name),
                   episodes_downloaded+j+1);
            printed_something = 1;
        }
    }

    if (!printed_something) puts("No new episodes\n");

    return 0;
}

/**
 * Print total count of all newly available episodes
 * @param anime_array json_object, must be of type json_type_array
 * @return -1 on error, otherwise 0
 */
int print_new_episodes_count(struct json_object * anime_array) {
    size_t i, n_anime, episodes_available, new_episodes;

    n_anime = json_object_array_length(anime_array);
    new_episodes = 0;

    for (i=0; i<n_anime; i++) {
        episodes_available = get_new_episodes_count(anime_array, i);
        new_episodes += episodes_available;
    }

    printf("%zu\n", new_episodes);

    return 0;
}

/**
 * Helper function to create an anime json object from provided values
 * @param name anime name
 * @param episodes anime's episode count
 * @param start_date anime's broadcast start date
 * @return a pointer to anime json object or NULL on error
 */
struct json_object * make_anime_json_object(char * name, size_t episodes, time_t start_date) {
    struct json_object * anime_obj = json_object_new_object();

    if (json_object_object_add(anime_obj, "name", json_object_new_string(name)) != 0) return NULL;
    if (json_object_object_add(anime_obj, "episodes", json_object_new_uint64(episodes)) != 0) return NULL;
    if (json_object_object_add(anime_obj, "episodes_downloaded", json_object_new_uint64(0)) != 0) return NULL;
    if (json_object_object_add(anime_obj, "start_date", json_object_new_uint64(start_date)) != 0) return NULL;
    if (json_object_object_add(anime_obj, "delayed_episodes", json_object_new_array()) != 0) return NULL;
    if (json_object_object_add(anime_obj, "ignored", json_object_new_boolean(0)) != 0) return NULL;

    return anime_obj;
}

/**
 * Helper function to make an anime json object with information provided though stdin
 * @return a pointer to anime json object or NULL on error
 */
struct json_object * make_anime_manual() {
    char anime_name[100]; // when changing size also change input format string in scanf()
    char anime_episodes_str[5]; // when changing size also change input format string in scanf()
    size_t anime_episodes;
    char time_str[17]; // when changing size also change input format string in scanf()
    int year, month, day, hours, minutes;
    time_t time_raw;
    struct tm * time_tm;

    puts("Adding anime manually");

    printf("Enter anime name: ");
    if (scanf("%99s", anime_name) != 1) {
        fprintf(stderr, "Failed to parse anime name\n");
        return NULL;
    }

    printf("Enter anime episodes count: ");
    if (scanf("%4s", anime_episodes_str) != 1) {
        fprintf(stderr, "Failed to parse anime episodes count\n");
        return NULL;
    }
    anime_episodes = strtoul(anime_episodes_str, NULL, 10);
    if (anime_episodes <= 0) {
        fprintf(stderr, "Failed to convert anime episodes count to unsigned long\n");
        return NULL;
    }

    time(&time_raw);
    time_tm = localtime(&time_raw);
    if (strftime(time_str, sizeof(time_str), "%F %R", time_tm) == 0) { //strftime appends a '\0' at the end automatically
        fprintf(stderr, "Failed to convert struct tm to string\n");
        return NULL;
    }
    printf("Enter anime broadcast start date (format: %s) (JST): ", time_str);
    while (getchar() != '\n');
    fgets(time_str, sizeof(time_str), stdin);
    memset(time_tm, 0, sizeof(struct tm));
    if (sscanf(time_str, "%d-%d-%d %d:%d", &year, &month, &day, &hours, &minutes) != 5) {
        fprintf(stderr, "Failed to convert string to struct tm\n");
        return NULL;
    }
    time_tm->tm_year = year - 1900;
    time_tm->tm_mon = month - 1; // months start from 0... why
    time_tm->tm_mday = day;
    time_tm->tm_hour = hours;
    time_tm->tm_min = minutes;
    setenv("TZ", "Asia/Tokyo", 1);
    tzset();
    time_raw = mktime(time_tm);

    return make_anime_json_object(anime_name, anime_episodes, time_raw);
}

/**
 * Create and add a new anime json object to the provided json array
 * @param anime_array json array to add the new anime json object to
 * @param method the method to use when creating a new anime json object
 * @return `0` if the anime was created and added successfully, otherwise `-1` on error
 */
int add_anime(struct json_object * anime_array, enum ADD_ANIME_METHOD method) {
    struct json_object * anime; // do not try to json_object_put() this

    switch (method) {
        case MANUAL:
            anime = make_anime_manual();
            break;
        default:
            return -1;
    }

    if (anime == NULL) {
        fprintf(stderr, "Failed to create anime manually\n");
        return -1;
    }

    json_object_array_add(anime_array, anime);

    return 0;
}

/**
 * Edit one anime field by providing input through stdin
 * @param anime anime to edit
 * @return 0 if the anime was edited successfully, otherwise -1 on error indicating that saving should not be performed
 */
int edit_anime(struct json_object * anime) {
    char choice_str[2];
    size_t choice;

    char anime_name[100];
    struct json_object * anime_name_obj;

    size_t episodes;
    struct json_object * episodes_obj;
    char episodes_str[5];

    struct tm * start_date;
    int year, month, day, hours, minutes;
    time_t start_date_raw;
    char start_date_str[17];
    struct json_object * start_date_obj;

    size_t i, delayed_episodes_length;
    size_t delayed_episode_temp;
    char delayed_episodes_str_buffer; // for getting input char by char
    int scanning = 1;
    char delayed_episodes_str[5]; // string to store input for scanning later
    struct json_object * delayed_episode_obj; // an array of delayed episodes
    struct json_object * delayed_episodes_obj;

    char ignored_str[6];
    struct json_object * ignored_obj;

    puts("Select the field to edit:");
    puts("\t1) Name");
    puts("\t2) Episodes count");
    puts("\t3) Downloaded episodes count");
    puts("\t4) Start date");
    puts("\t5) Delayed episodes");
    puts("\t6) Ignored flag");

    printf("Enter your choice: ");
    if (scanf("%1s", choice_str) != 1) {
        fprintf(stderr, "Failed to parse choice\n");
        return -1;
    }
    choice = strtoul(choice_str, NULL, 10);
    if (choice <= 0 || choice > 6) {
        fprintf(stderr, "Failed to select the field to edit\n");
        return -1;
    }

    switch (choice) {
        case 1: // name
            if (!json_object_object_get_ex(anime, "name", &anime_name_obj)) {
                fprintf(stderr, "Failed to get current anime name\n");
                return -1;
            }

            printf("Current anime name: %s\n", json_object_get_string(anime_name_obj));
            printf("Enter new anime name: ");
            if (scanf("%99s", anime_name) != 1) {
                fprintf(stderr, "Failed to read new anime name\n");
                return -1;
            }

            if (!json_object_set_string(anime_name_obj, anime_name)) {
                fprintf(stderr, "Failed to set new anime name\n");
                return -1;
            }
            break;
        case 2: // episodes
            if (!json_object_object_get_ex(anime, "episodes", &episodes_obj)) {
                fprintf(stderr, "Failed to get current anime episodes count\n");
                return -1;
            }

            printf("Current anime episodes count: %zu\n", json_object_get_uint64(episodes_obj));
            printf("Enter new anime episodes count: ");
            if (scanf("%4s", episodes_str) != 1) {
                fprintf(stderr, "Failed to read new anime episodes count\n");
                return -1;
            }
            episodes = strtoul(episodes_str, NULL, 10);
            if (episodes <= 0 || episodes > 9999) {
                fprintf(stderr, "Failed to parse new anime episodes count\n");
                return -1;
            }

            if (!json_object_set_uint64(episodes_obj, episodes)) {
                fprintf(stderr, "Failed to set new anime episodes count\n");
                return -1;
            }
            break;
        case 3: // episodes downloaded
            if (!json_object_object_get_ex(anime, "episodes_downloaded", &episodes_obj)) {
                fprintf(stderr, "Failed to get current anime downloaded episodes count\n");
                return -1;
            }

            printf("Current anime downloaded episodes count: %zu\n", json_object_get_uint64(episodes_obj));
            printf("Enter new anime downloaded episodes count: ");
            if (scanf("%4s", episodes_str) != 1) {
                fprintf(stderr, "Failed to read new anime downloaded episodes count\n");
                return -1;
            }
            episodes = strtoul(episodes_str, NULL, 10);
            if (episodes <= 0 || episodes > 9999) {
                fprintf(stderr, "Failed to parse new anime downloaded episodes count\n");
                return -1;
            }

            if (update_anime(anime, episodes) != 0) {
                fprintf(stderr, "Failed to set new anime downloaded episodes count\n");
                return -1;
            }
            break;
        case 4: // start date
            if (!json_object_object_get_ex(anime, "start_date", &start_date_obj)) {
                fprintf(stderr, "Failed to get current anime start date\n");
                return -1;
            }

            start_date_raw = json_object_get_int64(start_date_obj);
            setenv("TZ", "Asia/Tokyo", 1);
            tzset();
            start_date = localtime(&start_date_raw);

            if (strftime(start_date_str, sizeof(start_date_str), "%F %R", start_date) == 0) {
                fprintf(stderr, "Failed to convert struct tm to string\n");
                return -1;
            }

            printf("Current anime broadcast start date (JST): %s\n", start_date_str);
            printf("Enter new anime broadcast start date (JST): ");
            while (getc(stdin) != '\n');
            fgets(start_date_str, sizeof(start_date_str), stdin);

            memset(start_date, 0, sizeof(struct tm));
            if (sscanf(start_date_str, "%d-%d-%d %d:%d", &year, &month, &day, &hours, &minutes) != 5) {
                fprintf(stderr, "Failed to convert string to struct tm\n");
                return -1;
            }
            start_date->tm_year = year - 1900;
            start_date->tm_mon = month - 1; // months start from 0... why
            start_date->tm_mday = day;
            start_date->tm_hour = hours;
            start_date->tm_min = minutes;
            start_date_raw = mktime(start_date);

            if (!json_object_set_int64(start_date_obj, start_date_raw)) {
                fprintf(stderr, "Failed to set new anime broadcast start date\n");
                return -1;
            }
            break;
        case 5: // delayed episodes
            if (!json_object_object_get_ex(anime, "delayed_episodes", &delayed_episodes_obj)) {
                fprintf(stderr, "Failed to get current anime delayed episodes\n");
                return -1;
            }

            delayed_episodes_length = json_object_array_length(delayed_episodes_obj);
            printf("Current anime delayed episodes: ");
            if (delayed_episodes_length != 0) {
                for (i=0; i<delayed_episodes_length-1; i++) {
                    delayed_episode_obj = json_object_array_get_idx(delayed_episodes_obj, i);
                    if (delayed_episode_obj == NULL) {
                        fprintf(stderr, "Failed to print current anime delayed episodes\n");
                        return -1;
                    }
                    printf("%zu, ", json_object_get_uint64(delayed_episode_obj));
                }
                // print last
                delayed_episode_obj = json_object_array_get_idx(delayed_episodes_obj, i);
                if (delayed_episode_obj == NULL) {
                    fprintf(stderr, "Failed to print current anime delayed episodes\n");
                    return -1;
                }
                printf("%zu\n", json_object_get_uint64(delayed_episode_obj));
            } else {
                puts("none");
            }

            delayed_episodes_str[sizeof(delayed_episodes_str)-1] = '\0';
            // clear current delayed episodes array TODO this makes saving after edit_anime() returns an error unsafe
            json_object_array_del_idx(delayed_episodes_obj, 0, delayed_episodes_length);
            printf("Enter new anime delayed episodes: ");
            while (getchar() != '\n'); // clear stdin
            while (scanning) {
                for (i=0; i < sizeof(delayed_episodes_str)-1; i++) {
                    delayed_episodes_str_buffer = (char) getchar();
                    if (delayed_episodes_str_buffer == ',') { // comma means the end of one episode number
                        delayed_episodes_str[i] = '\0';
                        break;
                    }
                    if (delayed_episodes_str_buffer == '\n') { // newline means the end of input
                        scanning = 0;
                        break;
                    }
                    if (delayed_episodes_str_buffer == ' '){ // skip whitespace
                        i--;
                        continue;
                    }

                    delayed_episodes_str[i] = delayed_episodes_str_buffer;
                }

                if (i != 0) { // if zero digits entered there is no need to parse
                    delayed_episode_temp = strtoul(delayed_episodes_str, NULL, 0);
                    if (delayed_episode_temp == 0) { // episode can't be zero, strtoul returns zero on error
                        fprintf(stderr, "Failed to convert '%s' to a number\n", delayed_episodes_str);
                        return -1;
                    }
                    json_object_array_add(delayed_episodes_obj, json_object_new_uint64(delayed_episode_temp));
                }
            }

            json_object_array_shrink(delayed_episodes_obj, 0);
            break;
        case 6: // ignored
            if (!json_object_object_get_ex(anime, "ignored", &ignored_obj)) {
                fprintf(stderr, "Failed to get current anime ignored flag\n");
                return -1;
            }

            printf("Current anime ignored flag: %s\n", json_object_get_boolean(ignored_obj) ? "True" : "False");
            printf("Enter new anime ignored flag: ");
            if (scanf("%5s", ignored_str) != 1) {
                fprintf(stderr, "Failed to read new anime ignored flag\n");
                return -1;
            }

            if (strcmp(ignored_str, "True") == 0) {
                if (!json_object_set_boolean(ignored_obj, 1)) {
                    fprintf(stderr, "Failed to set new anime ignored flag\n");
                    return -1;
                }
                break;
            } else if (strcmp(ignored_str, "False") == 0) {
                if (!json_object_set_boolean(ignored_obj, 0)) {
                    fprintf(stderr, "Failed to set new anime ignored flag\n");
                    return -1;
                }
                break;
            } else {
                fprintf(stderr, "Failed to parse new anime ignored flag\n");
                return -1;
            }
            break;
        default:
            fprintf(stderr, "Bad choice number\n");
            return -1;
    }

    return 0;
}

/**
 * Delete an anime from anime array
 * @param anime_array anime array to delete the anime from
 * @param delete_at index of the anime to delete, starting from 0
 * @return 0 if the anime was deleted successfully, otherwise -1 on error
 */
int delete_anime(struct json_object * anime_array, size_t delete_at) {
    if (json_object_array_length(anime_array) <= delete_at) {
        fprintf(stderr, "Failed to delete anime with id %zu, no such anime exists\n", delete_at+1);
        return -1;
    }

    if (json_object_array_del_idx(anime_array, delete_at, 1) != 0) {
        fprintf(stderr, "Failed to delete anime with id %zu\n", delete_at+1);
        return -1;
    }

    json_object_array_shrink(anime_array, 0);

    return 0;
}

/**
 * Update anime's downloaded episodes count
 * @param anime anime to update
 * @param downloaded_episodes new downloaded episodes count
 * @return 0 on success, otherwise -1 on error
 */
int update_anime(struct json_object * anime, size_t downloaded_episodes) {
    struct json_object * episodes_obj;
    struct json_object * downloaded_episodes_obj;

    if (!json_object_object_get_ex(anime, "episodes", &episodes_obj)) {
        fprintf(stderr, "Failed to get anime episodes count\n");
        return -1;
    }
    if (!json_object_object_get_ex(anime, "episodes_downloaded", &downloaded_episodes_obj)) {
        fprintf(stderr, "Failed to get anime downloaded episodes count\n");
        return -1;
    }

    if (json_object_get_uint64(episodes_obj) < downloaded_episodes) {
        fprintf(stderr, "Failed to set new downloaded episodes count, it can't be bigger than anime's episode count\n");
        return -1;
    }

    if (!json_object_set_uint64(downloaded_episodes_obj, downloaded_episodes)) {
        fprintf(stderr, "Failed to set new anime downloaded episodes count\n");
        return -1;
    }

    return 0;
}

/**
 * Update anime's downloaded episodes count by incrementing it by 1
 * @param anime anime to update
 * @return 0 on success, otherwise -1 on error
 */
int update_anime_quick(struct json_object * anime) {
    struct json_object * downloaded_episodes_obj;

    if (!json_object_object_get_ex(anime, "episodes_downloaded", &downloaded_episodes_obj)) {
        fprintf(stderr, "Failed to get anime downloaded episodes count\n");
        return -1;
    }

    return update_anime(anime, json_object_get_uint64(downloaded_episodes_obj) + 1);
}

/**
 * Toggle anime's ignored flag
 * @param anime anime to edit
 * @return 0 on success, otherwise -1 on error
 */
int toggle_anime_ignored(struct json_object * anime) {
    struct json_object * ignored_obj;

    if (!json_object_object_get_ex(anime, "ignored", &ignored_obj)) {
        fprintf(stderr, "Failed to get anime ignored flag\n");
        return -1;
    }

    if (!json_object_set_boolean(ignored_obj, !json_object_get_boolean(ignored_obj))) {
        fprintf(stderr, "Failed to set new anime ignored flag\n");
        return -1;
    }

    return 0;
}

#include <json.h>
#include <stdio.h>
#include <time.h>

/**
 * List all saved anime
 * @param anime_array json_object, must be of type json_type_array
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
    char start_string[15];

    printf("%3c | %-30.30s | %-8.8s | %-15.15s\n", '#', "Anime name", "Episodes", "Broadcast");
    for (i=0; i<65; i++) putchar('-');
    putchar('\n');

    n_anime = json_object_array_length(anime_array);

    for (i=0; i<n_anime; i++) {
        anime = json_object_array_get_idx(anime_array, i);
        if (!json_object_object_get_ex(anime, "name", &anime_name)) {
            fprintf(stderr, "Malformed json");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "episodes", &anime_episodes)) {
            fprintf(stderr, "Malformed json");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "episodes_downloaded", &anime_episodes_downloaded)) {
            fprintf(stderr, "Malformed json");
            return -1;
        }
        if (!json_object_object_get_ex(anime, "start_date", &anime_start_date)) {
            fprintf(stderr, "Malformed json");
            return -1;
        }
        start_unix = json_object_get_int64(anime_start_date);
        start_datetime = localtime(&start_unix);
        strftime(start_string, sizeof(start_string), "%A %H:%M", start_datetime);
        printf("%3zu | %-30.30s | %3d/%-3d | %-15.15s\n",
               i+1,
               json_object_get_string(anime_name),
               json_object_get_int(anime_episodes_downloaded),
               json_object_get_int(anime_episodes),
               start_string);
    }

    for (i=0; i<65; i++) putchar('-');
    putchar('\n');
    return 0;
}

void show_new_episodes(struct json_object * anime_array) {

}

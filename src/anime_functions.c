#include <json.h>
#include <stdio.h>
#include <time.h>

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
    char start_string[15];

    printf("%3c | %-30.30s | %-8.8s | %-15.15s\n", '#', "Anime name", "Episodes", "Broadcast");
    for (i=0; i<65; i++) putchar('-');
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
        if (episodes_available < 0) return -1;

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
        if (episodes_available < 0) return -1;
        new_episodes += episodes_available;
    }

    printf("%zu\n", new_episodes);

    return 0;
}

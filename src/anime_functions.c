#ifndef JSON_C
#define JSON_C
#include <json.h>
#include <stdio.h>

#endif //JSON_C

/**
 * List all saved anime
 * @param anime_array json_object, must be of type json_type_array
 */
void list_all(struct json_object * anime_array) {
    size_t i, n_anime;
    struct json_object * anime;
    struct json_object * anime_name;

    n_anime = json_object_array_length(anime_array);

    for (i=0; i<n_anime; i++) {
        anime = json_object_array_get_idx(anime_array, i);
        if (!json_object_object_get_ex(anime, "name", &anime_name)) {
            fprintf(stderr, "Malformed json");
            return;
        }
        printf("%lu. %s\n",i+1,json_object_get_string(anime_name));
    }
}
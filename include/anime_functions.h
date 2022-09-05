#ifndef AWEEK_C_ANIME_FUNCTIONS_H
#define AWEEK_C_ANIME_FUNCTIONS_H
enum ADD_ANIME_METHOD { //TODO MAL url parsing
    MANUAL,
};
int list_all(struct json_object * anime_array);
int print_new_episodes(struct json_object * anime_array);
int print_new_episodes_count(struct json_object * anime_array);
int add_anime(struct json_object * anime_array, enum ADD_ANIME_METHOD method);
int edit_anime(struct json_object * anime);
int delete_anime(struct json_object * anime_array, size_t delete_at);
int update_anime(struct json_object * anime, size_t downloaded_episodes);
int update_anime_quick(struct json_object * anime);
int toggle_anime_ignored(struct json_object * anime);
#endif //AWEEK_C_ANIME_FUNCTIONS_H

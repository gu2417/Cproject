#ifndef USER_STORE_H
#define USER_STORE_H

void user_store_init(void);
int user_exists(const char *user_id);
int user_verify_password(const char *user_id, const char *password);
int user_get_profile(const char *user_id, char *nickname, char *profile_msg);
int settings_get(const char *user_id, char *theme);
int settings_update(const char *user_id, const char *theme);
int profile_update(const char *user_id, const char *nickname, const char *profile_msg);

#endif // USER_STORE_H

#ifndef USER_H
#define USER_H

enum avatars {
    AVATAR_1 = 1,
    AVATAR_2,
    AVATAR_3,
    AVATAR_4,
    AVATAR_5,
    AVATAR_6,
    AVATAR_7,
    AVATAR_8,
    AVATAR_9,
    AVATAR_10,
    AVATAR_11,
    AVATAR_12,
    AVATAR_13,
    AVATAR_14,
    AVATAR_15,
    AVATAR_16
};

enum avatars getAvatarFromNumber(int number);

typedef struct {
    char* email;
    char* username;
    char* password;
    enum avatars avatar;
} User;

#endif /* USER_H */
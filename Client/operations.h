//
// Created by hudec on 1. 1. 2022.
//

#ifndef SEMESTRALKA_OPERATIONS_H
#define SEMESTRALKA_OPERATIONS_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "../maxLimits.h"

typedef struct friendList {
    char **friends;
    int friendsCount;
} FriendList;

bool registerUser(int sockfd);
bool loginUser(int sockfd, FriendList *friendsList);
bool logoutUser(int sockfd);
bool addFriend(int sockfd, FriendList *friendslist);
bool deleteFriend(int sockfd, FriendList *friendsList);
bool deleteAccount(int sockfd);
bool sendMessage(int sockfd, FriendList *friendsList);

bool showRequests(int sockfd, FriendList *friendslist);

#endif //SEMESTRALKA_OPERATIONS_H

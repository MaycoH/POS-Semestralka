#ifndef SEMESTRALKA_USERS_H
#define SEMESTRALKA_USERS_H

#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "../maxLimits.h"

/** Štruktúra obsahujúca odosielateľa a správu */
typedef struct {
    char *sentFrom;
    char *messageText;
} Message;

typedef struct {
    char *nickname;             // Dvaja s rovnakym nickom nemozu byt v chate
    char *password;
    char **friends;       // Nick je unikatny, mame len 1 zoznam registrovanych
    int friendsCount;
    char **addFriendRequest;
    int addFriendRequestCount;
    char **deleteFriendRequest;     // O zrusenie priatelstva
    int deleteFriendRequestCount;
    int socketNr;
    int messagesCount;
    Message **messages;
} User;

typedef struct {
    char *buffer;
    char *nickname; // temp
    char *password; // temp
    char *messageFrom;
    char *messageTo;
    int status;
    char *option;
    char *messsage; // Navratova sprava pouzivatelovi
    char *userMessage;
    int socketID;
    int index;
    User **registeredUsers;
    User **loggedInUsers;
} ClientData;

typedef struct {
    unsigned long threadID;
    bool threadEnded;
    ClientData* my_client;
    pthread_mutex_t *mutexRegister;
    pthread_mutex_t *mutexLogin;
    pthread_mutex_t *mutexAddFriendRequest;
    pthread_mutex_t *mutexDeleteFriendRequest;
    pthread_mutex_t *mutexMessages;
} ThreadData;

void *findUser(char *nickName, User **usersList);
void *isUserLoggedIn(char *nickName, User **usersList);

/** Funkcia pre obsluhu klientov prihlásených do aplikácie
 * @param data Dáta, s ktorými má vlákno pracovať
 */
void *obsluzKlienta(void *data);

// Privátne pomocné metódy pre metódu "obsluzKlienta"
void loginUser(int *n, ThreadData *threadData, ClientData *clientData, User **loggedUser);
void registerUser(int *n, ThreadData *threadData, ClientData *clientData);
void logoutUser(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);
void deleteUser(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);
void addFriend(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);
void cancelFriend(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);
void messages(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);



//class users{
//
//};


#endif //SEMESTRALKA_USERS_H

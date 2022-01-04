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
    char *nickname;
    char *password;
    char **friends;
    int friendsCount;
    char **addFriendRequest;
    int addFriendRequestCount;
    char **deleteFriendRequest;
    int deleteFriendRequestCount;
    int socketNr;
    int messagesCount;
    Message **messages;
} User;

typedef struct {
    char *nickname; // dočasná premenná
    char *password; // dočasná premenná
    int status;
    int socketID;
    int clientID;
    User **registeredUsers;
    User **loggedInUsers;
} ClientData;

typedef struct {
    unsigned long threadID;
    bool threadEnded;
    ClientData* clientData;
    pthread_mutex_t *mutexRegister;
    pthread_mutex_t *mutexLogin;
    pthread_mutex_t *mutexAddFriendRequest;
    pthread_mutex_t *mutexDeleteFriendRequest;
    pthread_mutex_t *mutexMessages;
} ThreadData;

/**
 * Funkcia pre vyhľadanie užívateľa v zozname užívateľov
 * @param nickName hľadaný nick
 * @param usersList zoznam, v ktorom sa má hľadať konkrétny užívateľ
 * @return užívateľa ak bol nájdený, ináč NULL
 */
void *findUser(char *nickName, User **usersList);

/** Funkcia pre obsluhu klientov prihlásených do aplikácie
 * @param data Dáta, s ktorými má vlákno pracovať
 */
void *clientRoutine(void *data);



// Privátne pomocné metódy pre metódu "clientRoutine"
/**
 * Funkcia pre prihlásenie užívateľa
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 * @param loggedUser prihlasovaný užívateľ (odkaz na objekt užívateľa, do ktorého sa majú uložiť info po prihlásení)
 */
void loginUser(int *n, ThreadData *threadData, ClientData *clientData, User **loggedUser);

/**
 * Funkcia pre registráciu nového užívateľa
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 */
void registerUser(int *n, ThreadData *threadData, ClientData *clientData);

/**
 * Funkcia pre odhlásenie prihláseného užívateľa
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 * @param loggedUser prihlásený užívateľ
 */
void logoutUser(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);

/**
 * Funkcia pre zmazanie prihláseného užívateľa
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 * @param loggedUser prihlásený užívateľ
 */
void deleteUser(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);

/**
 * Funkcia pre pridanie priateľa
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 * @param loggedUser prihlásený užívateľ */
void addFriend(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);

/**
 * Funkcia pre zrušenie priateľstva
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 * @param loggedUser prihlásený užívateľ
 */
void cancelFriend(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);

/**
 * Funkcia pre správu správ prihláseného užívateľa
 * @param n premenná, v ktorej sa uchovávajú stavy funkcií Read a Write pri komunikácii
 * @param threadData dáta vlákna
 * @param clientData dáta klienta
 * @param loggedUser prihlásený užívateľ
 */
void messages(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser);

#endif //SEMESTRALKA_USERS_H
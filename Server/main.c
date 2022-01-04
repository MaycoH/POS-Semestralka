#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#include "users.h"
#include "../colors.h"



bool run = false;
int threadsCount = 0;
int connectedClientsCount = 0;


int main(int argc, char *argv[]) {

    // Príprava sieťovo komunikačných vecí
    int sockfd, newsockfd;                      // sockfd = ID socketu, na ktorom počúva; newsockfd = ID klientského socketu
    socklen_t cli_len;                          // Dĺžka socketu klienta
    struct sockaddr_in serv_addr, cli_addr;     // Štruktúra s adresami: serverová a klientská
    int n;
    char buffer[256];                           // Buffer so správou

    if (argc < 2) {           //  Skontrolujeme či máme dostatok argumentov. SERVER: len port, CLIENT: IP a port
        fprintf(stderr,RED"Usage: %s port\n"RESET, argv[0]);
        return 1;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));                 // Vynulujeme a zinicializujeme sieťovú adresu.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;                            // Chceme počúvať akúkoľvek adresu
    serv_addr.sin_port = htons(atoi(argv[1]));          // Nastavíme port, na ktorom chceme počúvať ("htons" konvertuje číslo  na sieťovú adresu)

    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //  Vytvoríme si socketID v doméne AF_INET. Vytvorí nám deskriptor socketu, pomocou ktorého bude server komunikovať. "SOCK_STREAM" = TCP.
    if (sockfd < 0) {
        perror(" *** Error creating socket\n");
        return 1;
    }

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {  //  Priradíme vyplnenú sieťovú adresu vytvorenému socketu.
        perror(RED " *** Error binding socket address" RESET);
        return 2;
    }
    printf(GREEN " *** Socket and Bind OK\n" RESET);

    listen(sockfd, 5);  //  Pripravíme socketID pre príjmanie spojení od klientov. Maximálna dĺžka fronty neobslúžených spojení je 5.

    // Príprava štruktúr - alokácia pamäte
    User **registeredUsers = malloc(sizeof (User*) * MAX_USERS_COUNT);
    User **loggedInUsers = malloc(sizeof (User*) * MAX_USERS_COUNT);
    ThreadData **threads = (ThreadData**) malloc(sizeof (ThreadData*) * MAX_USERS_COUNT);
    ClientData **connectedClients = malloc(sizeof (ClientData*) * MAX_USERS_COUNT);

    // Príprava mutexov
    pthread_mutex_t mutexRegister;
    pthread_mutex_t mutexLogin;
    pthread_mutex_t mutexAddFriendRequests;
    pthread_mutex_t mutexDeleteFriendRequests;
    pthread_mutex_t mutexFriendsCount;
    pthread_mutex_t mutexThreads;
    pthread_mutex_t mutexMessages;

    pthread_mutex_init(&mutexRegister, NULL);
    pthread_mutex_init(&mutexLogin, NULL);
    pthread_mutex_init(&mutexAddFriendRequests, NULL);
    pthread_mutex_init(&mutexDeleteFriendRequests, NULL);
    pthread_mutex_init(&mutexFriendsCount, NULL);
    pthread_mutex_init(&mutexThreads, NULL);
    pthread_mutex_init(&mutexMessages, NULL);

    run = true;

    while (run) {
        // kontrola vlákien - upratanie
        for (int i = 0; i < threadsCount; ++i) {
            ThreadData *thrData = threads[i];
            // Upratanie dát po skončení vlákna
            if (thrData != NULL) {
                if (thrData->threadEnded) {
                    printf("Ukončujem vlákno %d/%d\n", i+1, threadsCount);
                    if (thrData->clientData->nickname == 0) {
                        free(thrData->clientData->nickname);
                        free(thrData->clientData->password);
                    }
                    pthread_mutex_lock(&mutexFriendsCount);
                    connectedClientsCount--;
                    connectedClients[thrData->clientData->clientID] = connectedClients[connectedClientsCount];
                    connectedClients[connectedClientsCount] = NULL;
                    pthread_mutex_unlock(&mutexFriendsCount);
                    free(thrData->clientData);
                    pthread_mutex_lock(&mutexThreads);
                    unsigned long threadID = thrData->threadID;       // Pre výpis uvoľňovaného ID vlákna
                    threads[i] = threads[threadsCount];
                    threads[threadsCount] = NULL;
                    // Uvoľnenie vlákna a upravanie pamäte
                    pthread_detach(thrData->threadID);
                    free(thrData);
                    threadsCount--;
                    pthread_mutex_unlock(&mutexThreads);
                    printf(GREEN " *** Vlákno %lu uvoľnené. Počet vlákien: %d\n" RESET, threadID, threadsCount);
                }
            }
        }

        // Prijatie spojenia od klienta
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);  //  Počkáme na a príjmeme spojenie od klienta.
        if (newsockfd < 0) {
            perror(RED "ERROR on accept\n" RESET);
            return 3;
        }
        printf(GREEN" *** Spojenie od klienta úspešne akceptované.\n" RESET);

        // Vytvorenie klienta pre pridanie do zoznamu (príprava klienta)
        ClientData *cliData = malloc(sizeof (ClientData));
        cliData->socketID = newsockfd;
        cliData->nickname = malloc(sizeof (char) * MAX_NICKNAME_LENGTH);
        cliData->password = malloc(sizeof (char) * MAX_PASSWORD_LENGTH);
        cliData->registeredUsers = registeredUsers;
        cliData->loggedInUsers = loggedInUsers;
        cliData->status = false;
        cliData->clientID = connectedClientsCount;
        // Pridanie klienta do zoznamu prihlásených klientov
        pthread_mutex_lock(&mutexFriendsCount);
        connectedClients[connectedClientsCount++] = cliData;   // vložím a až potom incrementnem
        pthread_mutex_unlock(&mutexFriendsCount);

        // Príprava vlákna pre klienta
        pthread_t thread;
        ThreadData *thrData = malloc(sizeof (ThreadData));
        thrData->threadID = thread;
        thrData->threadEnded = false;
        thrData->clientData = cliData;
        // Príprava mutexov pre klienta
        thrData->mutexRegister = &mutexRegister;
        thrData->mutexLogin = &mutexLogin;
        thrData->mutexMessages = &mutexMessages;
        thrData->mutexAddFriendRequest = &mutexAddFriendRequests;
        thrData->mutexDeleteFriendRequest = &mutexDeleteFriendRequests;
        // Pridanie vlákna do zoznamu vlákien
        pthread_mutex_lock(&mutexThreads);
        threads[threadsCount++] = thrData;
        pthread_mutex_unlock(&mutexThreads);

        // Vytvorenie samotného vlákna
        printf("Vytváram a obsluhujem vlákno %lu\n", thrData->threadID);
        pthread_create(&thread, NULL, &clientRoutine, thrData);
    }

    // Zrušenie vytvorených mutexov
    pthread_mutex_destroy(&mutexRegister);
    pthread_mutex_destroy(&mutexLogin);
    pthread_mutex_destroy(&mutexAddFriendRequests);
    pthread_mutex_destroy(&mutexDeleteFriendRequests);
    pthread_mutex_destroy(&mutexFriendsCount);
    pthread_mutex_destroy(&mutexThreads);
    pthread_mutex_destroy(&mutexMessages);
    // Zatvorenie oboch socketov, ktoré sme si vytvorili.
    close(sockfd);
    return 0;
}

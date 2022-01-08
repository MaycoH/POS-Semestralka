#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#include "users.h"

bool run = false;
int threadsCount = 0;
int connectedClientsCount = 0;
ThreadData  **threads = NULL;
ClientData **connectedClients = NULL;
ThreadData *thrData = NULL;
ClientData *cliData = NULL;
pthread_mutex_t mutexThreads;
pthread_mutex_t mutexClients;

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
    printf(GREEN " *** Socket and Bind OK" RESET "\n");

    listen(sockfd, 5);  //  Pripravíme socketID pre príjmanie spojení od klientov. Maximálna dĺžka fronty neobslúžených spojení je 5.

    // Príprava štruktúr - alokácia pamäte
    User **registeredUsers = malloc(sizeof (User*) * MAX_USERS_COUNT);
    User **loggedInUsers = malloc(sizeof (User*) * MAX_USERS_COUNT);
    threads = malloc(sizeof (ThreadData*) * MAX_USERS_COUNT);
    connectedClients = malloc(sizeof (ClientData*) * MAX_USERS_COUNT);

    // Príprava mutexov
    pthread_mutex_t mutexRegister;
    pthread_mutex_t mutexLogin;

    pthread_mutex_t mutexAddFriendRequests;
    pthread_mutex_t mutexDeleteFriendRequests;
    pthread_mutex_t mutexMessages;

    pthread_mutex_init(&mutexRegister, NULL);
    pthread_mutex_init(&mutexLogin, NULL);
    pthread_mutex_init(&mutexAddFriendRequests, NULL);
    pthread_mutex_init(&mutexDeleteFriendRequests, NULL);
    pthread_mutex_init(&mutexClients, NULL);
    pthread_mutex_init(&mutexThreads, NULL);
    pthread_mutex_init(&mutexMessages, NULL);

    run = true;

    while (run) {
        // Prijatie spojenia od klienta
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);  //  Počkáme na a príjmeme spojenie od klienta.
        if (newsockfd < 0) {
            perror(RED "ERROR on accept\n" RESET);
            return 3;
        }
        printf(GREEN" *** Spojenie od klienta úspešne akceptované."RESET"\n" );
        pthread_attr_t attr;            // Vytvoríme atribút

        if (!run) {

            for (int i = 0; i < registeredCount; ++i) {

                free(registeredUsers[i]->nickname);
                free(registeredUsers[i]->password);

                for (int j = 0; j < registeredUsers[i]->addFriendRequestCount; ++j) {
                    free(registeredUsers[i]->addFriendRequest[j]);
                }
                free(registeredUsers[i]->addFriendRequest);

                for (int j = 0; j < registeredUsers[i]->deleteFriendRequestCount; ++j) {
                    free(registeredUsers[i]->deleteFriendRequest[j]);
                }
                free(registeredUsers[i]->deleteFriendRequest);

                for (int j = 0; j < registeredUsers[i]->friendsCount; ++j) {
                    free(registeredUsers[i]->friends[j]);
                }
                free(registeredUsers[i]->friends);

                for (int j = 0; j < registeredUsers[i]->messagesCount; ++j) {
                    free(registeredUsers[i]->messages[j]->sentFrom);
                    free(registeredUsers[i]->messages[j]->messageText);
                    free(registeredUsers[i]->messages[j]);
                }
                free(registeredUsers[i]->messages);

                free(registeredUsers[i]);
            }

            pthread_attr_destroy(&attr);
            // Zrušenie vytvorených mutexov
            pthread_mutex_destroy(&mutexRegister);
            pthread_mutex_destroy(&mutexLogin);
            pthread_mutex_destroy(&mutexAddFriendRequests);
            pthread_mutex_destroy(&mutexDeleteFriendRequests);
            pthread_mutex_destroy(&mutexClients);
            pthread_mutex_destroy(&mutexThreads);
            pthread_mutex_destroy(&mutexMessages);
            free(registeredUsers);
            free(loggedInUsers);
            free(connectedClients);

            for (int i = 0; i < threadsCount; ++i) {
                free(threads[i]);
            }
            free(threads);

            // Zatvorenie oboch socketov, ktoré sme si vytvorili.
            close(sockfd);
            pthread_exit(0);
            return 0;
        }

        // Vytvorenie klienta pre pridanie do zoznamu (príprava klienta)
        cliData= malloc(sizeof(ClientData));
        cliData->socketID = newsockfd;
        cliData->registeredUsers = registeredUsers;
        cliData->loggedInUsers = loggedInUsers;
        cliData->status = false;
        cliData->clientID = connectedClientsCount;
        // Pridanie klienta do zoznamu prihlásených klientov
        pthread_mutex_lock(&mutexClients);
        connectedClients[connectedClientsCount++] = cliData;   // vložím a až potom incrementnem
        pthread_mutex_unlock(&mutexClients);

        // Príprava vlákna pre klienta
        pthread_t thread = 0;

        pthread_attr_init(&attr);       // inicializujeme atribút
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);    // nastavíme atribútu, že bude defaultne detachnutý (defaultne je joinovateľný)

        thrData = malloc(sizeof(ThreadData));
        thrData->threadID = thread;
        thrData->threadEnded = false;
        thrData->clientData = cliData;
        // Príprava mutexov pre klienta
        thrData->mutexRegister = &mutexRegister;
        thrData->mutexLogin = &mutexLogin;
        thrData->mutexMessages = &mutexMessages;
        thrData->mutexAddFriendRequest = &mutexAddFriendRequests;
        thrData->mutexDeleteFriendRequest = &mutexDeleteFriendRequests;
        thrData->threadsCount = threadsCount;
        // Pridanie vlákna do zoznamu vlákien
        pthread_mutex_lock(&mutexThreads);
        threads[threadsCount++] = thrData;
        pthread_mutex_unlock(&mutexThreads);

        pthread_attr_init(&attr);       // inicializujeme atribút
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);    // nastavíme atribútu, že bude defaultne detachnutý (defaultne je joinovateľný)

        // Vytvorenie samotného vlákna
        printf("Vytváram a obsluhujem vlákno\n");
        pthread_create(&thread, &attr, &clientRoutine, thrData);
    }
}

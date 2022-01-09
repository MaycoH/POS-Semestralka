#include "users.h"

int registeredCount = 0;
int loggedInCount = 0;
void *findUser(char *nickName, User **usersList) {
    for (int i = 0; i < registeredCount; ++i) {
        if (strcmp(nickName, usersList[i]->nickname) == 0) {    // Ak hľadaný nick existuje niekde v zozname užívateľov, tak ho vráť, ináč vráť NULL.
            return usersList[i];
        }
    }
    return NULL;
}

void *clientRoutine(void *data) {
    int n;
    bool ended = false;
    char buffer[256];
    char message[256];
    User *loggedUser;
    ThreadData *thrData = data;
    ClientData *client = thrData->clientData;
    printf("Vytvorené vlákna: %d\n", threadsCount);
    while (!ended) {
        // AK všetko prebehlo bezproblémovo, môžeme začať s klientom komunikovať
        n = 0;
        bzero(buffer, 256); // Vyčistenie bufferu
        n = read(client->socketID, buffer, 255);   //  Načítame správu od klienta cez socketID do buffra.
        if (n < 0)
        {
            perror(RED"*** Error reading from socket"RESET);
            return (void *) 4;
        }

        char *options = "Možnosti: Login (L), Registrácia (R), Odhlásenie(O), zmazanie užívateľa (D), pridanie priateľa (A), zrušenie priateľa (?), zobrazenie správy (M)\n";
        char *option = strtok(buffer, "|");

        if (option != 0) {
            // Možnosti: Login (L), Registrácia (R), Odhlásenie(O), zmazanie užívateľa (D), pridanie priateľa (A), zrušenie priateľa (?), zobrazenie správy (M)
            switch (*option) {
                case 'L':
                    loginUser(&n, thrData, client, &loggedUser);
                    break;
                case 'R':
                    registerUser(&n, thrData, client);
                    break;
                case 'O':
                    logoutUser(&n, thrData, client, loggedUser);
                    break;
                case 'D':
                    deleteUser(&n, thrData, client, loggedUser);
                    break;
                case 'A':
                    addFriend(&n, thrData, client, loggedUser);
                    break;
                case 'C':
                    cancelFriend(&n, thrData, client, loggedUser);
                    break;
                case 'M':
                    messages(&n, thrData, client, loggedUser);
                    break;
                case 'F':
                    ended = true;
                    printf("Klient ukončil spojenie.\n");
                    break;
                default: {  // Zvolená neplatná možnosť
                    sprintf(message, "Zvolená neplatná možnosť: %s\n", option);
                    printf("%s", message);
                    strcat(message, options);
                    n = write(client->socketID, message, strlen(message) + 1);
                    break;
                }
            }
            if (n < 0)
            {
                perror("Error reading from or writing to socket");
                return false;
            }
        } else {
            ended = true;
            printf("Klient ukončil spojenie.\n");
        }
    }

    thrData->threadEnded = true;    // Nastavím vlákno ako skončené, nech sa v MAINe uprace
    close(client->socketID);    // ukončím socket

    int sid = client->socketID;
    // Ukončenie spojenia s klientom
    pthread_mutex_lock(&mutexClients);
    connectedClientsCount--;
    connectedClients[thrData->clientData->clientID] = connectedClients[connectedClientsCount];
    connectedClients[connectedClientsCount] = NULL;
    pthread_mutex_unlock(&mutexClients);

    pthread_mutex_lock(&mutexThreads);
    unsigned long threadID = thrData->threadID;       // Pre výpis uvoľňovaného ID vlákna
    // Uvoľnenie vlákna a upravanie pamäte
    free(thrData->clientData);
    free(thrData);
    threadsCount--;
    pthread_mutex_unlock(&mutexThreads);

    printf(GREEN " *** Vlákno uvoľnené. Počet vlákien: %d" RESET"\n", threadsCount);
    if (connectedClientsCount == 0) run = false;
    pthread_detach(threadID);
}

void loginUser(int *n, ThreadData *threadData, ClientData *clientData, User **loggedUser) {
    char buffer[256];
    char message[256];
    char *login = strtok(NULL, "|");
    char *password = strtok(NULL, "|");
    printf(YELLOW "*** Prihlasujem užívateľa \"%s\"" RESET"\n", login);
    clientData->status = 0;
    bool found = false;
    bool foundNick = false;

    for (int i = 0; i < registeredCount; ++i) {     // Postupne prechádzam cez všetkých užívateľov a potovnávam ich mená a heslá
        if (strcmp(login, clientData->registeredUsers[i]->nickname) == 0) {      // Meno je OK
            printf(GREEN "\t*** NickName je OK"RESET"\n");
            foundNick = true;
            if (strcmp(password, clientData->registeredUsers[i]->password) == 0) {  // aj heslo je OK
                printf(GREEN "\t*** Heslo je OK" RESET "\n");

                // Samotné prihlásenie
                pthread_mutex_lock(threadData->mutexLogin);
                pthread_mutex_lock(threadData->mutexRegister);
                clientData->loggedInUsers[loggedInCount] = clientData->registeredUsers[i];
                *loggedUser = clientData->registeredUsers[i];
                loggedInCount++;
                clientData->status = loggedInCount;
                pthread_mutex_unlock(threadData->mutexRegister);
                pthread_mutex_unlock(threadData->mutexLogin);
                found = true;
                break;
            }
        }
    }
    if (!foundNick) printf(RED "\t*** NickName je nesprávny"RESET"\n");
    if(!found) printf(RED "\t*** Heslo je nesprávne"RESET"\n");

    // Po prihlásení OK/NG odpoveď klientovi
    if (clientData->status != 0) {                 // OK prihlásenie
        strcpy(message, "Login prebehol úspešne\n");
        *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
        usleep(500000);     // Počkám pol sekundy
        pthread_mutex_lock(threadData->mutexLogin);
        *loggedUser = clientData->loggedInUsers[clientData->status - 1];
        printf("Posielam žiadosti o priateľstvo pri prihlásení (počet: %d)\n", (*loggedUser)->addFriendRequestCount);
        // Spracovanie žiadostí o priateľstvo
        if ((*loggedUser)->addFriendRequestCount > 0) {
            bzero(message, 256);
            sprintf(message, "Žiadosti: %d", (*loggedUser)->addFriendRequestCount);
        } else {
            strcpy(message, "Žiadne nové žiadosti\n");
        }
        pthread_mutex_unlock(threadData->mutexLogin);
        // Poslanie správy klientovi
        *n = write(clientData->socketID, message, strlen(message)+1);     //  Pošleme odpoveď klientovi.
        usleep(500000);     // Počkám pol sekundy

        // Zobrazenie žiadostí o priateľstvo po prihlásení
        char *requestingFriend = "";

        int friendRequestsCount = (*loggedUser)->addFriendRequestCount;
        for (int i = 0; i < friendRequestsCount; ++i) {     // Postupne prebehnem všetky žiadosti o priateľstvo daného užívateľa
            pthread_mutex_lock(threadData->mutexAddFriendRequest);
            requestingFriend = (*loggedUser)->addFriendRequest[i];  // Vytiahnem si meno žiadajúceho priateľa
            pthread_mutex_unlock(threadData->mutexAddFriendRequest);
            // Poslanie žiadosti
            sprintf(message, "%s", requestingFriend);
            *n = write(clientData->socketID, message, strlen(message)+1);       //  Pošleme odpoveď klientovi.
            printf("Poslaná žiadosť o priateľstvo užívateľovi %s od užívateľa %s, čakám na potvrdenie.\n", (*loggedUser)->nickname, requestingFriend);;
            // Prijatie odpovede
            bzero(buffer,256);                                        //  Vyčístíme buffer
            *n = read(clientData->socketID, buffer, 255);   //  Načítame správu od klienta cez socket do buffra.
            printf("Prijatá odpoveď na žiadosť o priateľstvo od užívateľa %s, odpoveď je: %s\n", (*loggedUser)->nickname, buffer);

            if (strcasecmp(buffer, "Y") == 0 || strcasecmp(buffer, "A") == 0) {
                printf("Pridávam užívateľa %s do zoznamu priateľov užívateľa %s.\n", requestingFriend, (*loggedUser)->nickname);
                pthread_mutex_lock(threadData->mutexLogin);
                (*loggedUser)->friends[(*loggedUser)->friendsCount++] = requestingFriend;       // Pridám žiadajúceho priateľa do zoznamu priateľov užívateľa (pole reťazcov)
                (*loggedUser)->addFriendRequestCount--;
                pthread_mutex_unlock(threadData->mutexLogin);

                pthread_mutex_lock(threadData->mutexRegister);
                User *requestingUser = findUser(requestingFriend, clientData->registeredUsers);
                pthread_mutex_unlock(threadData->mutexRegister);
                // Pridanie užívateľa do priateľov žiadajúceho užívateľa
                requestingUser->friends[requestingUser->friendsCount] = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);
                strcpy(requestingUser->friends[requestingUser->friendsCount], (*loggedUser)->nickname);     // Pridanie užívateľa na koniec jeho zoznamu
                requestingUser->friendsCount++;
                pthread_mutex_unlock(threadData->mutexLogin);

                sprintf(message, "Užívateľ úspešne pridaný");
                printf("Užívateľ %s úspešne pridaný do zoznamu priateľov užívateľa %s. Počet priateľov: %d\n", requestingUser->nickname, (*loggedUser)->nickname, (*loggedUser)->friendsCount);
                *n = write(clientData->socketID, message, strlen(message)+1);
                usleep(200000);
            }
        }
    } else {        // Neúspešné prihlásenie
        printf("Neúspešný login");
        strcpy(message, "Login prebehol neúspešne\n");
        *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
        if (*n < 0)
            perror(RED"Error writing to socket\n"RESET);
    }
}

void registerUser(int *n, ThreadData *threadData, ClientData *clientData) {
    char message[256];
    // Rozparsujem prijatý reťazec
    clientData->nickname = strtok(NULL, "|");
    clientData->password = strtok(NULL, "|");
        printf(YELLOW "*** Registrujem užívateľa \"%s\", heslo je: %s"RESET"\n", clientData->nickname, clientData->password);

    clientData->status = 1;    // 1 - Zaregistrovaný, 2 - Nájdená duplicita užívateľa počas registrácie
    // Hľadanie, či užívateľ s daným nickom už existuje
    pthread_mutex_lock(threadData->mutexRegister);
    for (int i = 0; i < registeredCount; ++i) {
        if (strcmp(clientData->nickname, clientData->registeredUsers[i]->nickname) == 0) {  // nájdený duplicitný užívateľ
            clientData->status = 2;
            pthread_mutex_unlock(threadData->mutexRegister);        // Unlock mutexu pri nájdení duplicity
            break;                                                         // Vyskončenie z cyklu
        }
    }
    if (clientData->status != 2) {     // Ak nebola nájdená duplicita
        pthread_mutex_unlock(threadData->mutexRegister);            // Unlock mutexu pri NEnájdení duplicity
    }

    // Samotná registrácia užívateľa
    if (clientData->status == 1) {
        // Pridanie užívateľa do zoznamu registrovaných užívateľov
        pthread_mutex_lock(threadData->mutexRegister);
        clientData->registeredUsers[registeredCount] = malloc(sizeof (User));

        clientData->registeredUsers[registeredCount]->nickname = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);
        strcpy(clientData->registeredUsers[registeredCount]->nickname, clientData->nickname);
        clientData->registeredUsers[registeredCount]->password = malloc(sizeof(char) * MAX_PASSWORD_LENGTH);
        strcpy(clientData->registeredUsers[registeredCount]->password, clientData->password);
        clientData->registeredUsers[registeredCount]->friends = malloc(sizeof(char*) * MAX_USERS_COUNT - 1);
        clientData->registeredUsers[registeredCount]->friendsCount = 0;
        clientData->registeredUsers[registeredCount]->addFriendRequest = malloc(sizeof(char*) * MAX_USERS_COUNT - 1);
        clientData->registeredUsers[registeredCount]->addFriendRequestCount = 0;
        clientData->registeredUsers[registeredCount]->deleteFriendRequest = malloc(sizeof(char*) * MAX_USERS_COUNT - 1);
        clientData->registeredUsers[registeredCount]->deleteFriendRequestCount = 0;
        clientData->registeredUsers[registeredCount]->messages = malloc(sizeof(Message *) * MAX_USERS_COUNT);
        clientData->registeredUsers[registeredCount]->messagesCount = 0;
        clientData->registeredUsers[registeredCount]->socketNr = clientData->socketID;
        registeredCount++;
        pthread_mutex_unlock(threadData->mutexRegister);
        strcpy(message, "Registrácia užívateľa úspešná.\n");
        printf("Login: %s, password: %s\n", clientData->registeredUsers[registeredCount - 1]->nickname, clientData->registeredUsers[registeredCount-1]->password);

    }
    else if (clientData->status == 2) {
        strcpy(message, "Užívateľ s daným menom už je zaregistrovaný!\n");
    }
    else strcpy(message, "Iná chyba pri registrácii.\n");

    printf("%s", message);

    // Zaslanie správy klientovi
    *n = write(clientData->socketID, message, strlen(message) + 1);
    if (*n < 0)
        perror(RED"Error writing to socket\n"RESET);
}

void logoutUser(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser) {
    char message[256];
    clientData->nickname = loggedUser->nickname;
    clientData->status = 0;
    printf("Odhlasujem užívateľa: %s... ", clientData->nickname);

    pthread_mutex_lock(threadData->mutexLogin);
    for (int i = 0; i < loggedInCount; ++i) {
        if (strcmp(clientData->nickname, clientData->loggedInUsers[i]->nickname) == 0) {        // Ak je užívateľ prihlásený
            // vymením ho s posledným a vynulujem
            if (i != loggedInCount - 1) {
                User *tempUser = clientData->loggedInUsers[i];
                clientData->loggedInUsers[i] = clientData->loggedInUsers[loggedInCount - 1];
                clientData->loggedInUsers[loggedInCount - 1] = tempUser;
            }
            clientData->loggedInUsers[loggedInCount - 1] = NULL;
            loggedInCount--;
            clientData->status = 1;
            printf("Odhlásený\n");
            break;
        }
    }
    pthread_mutex_unlock(threadData->mutexLogin);

    if (clientData->status == 1) {
        strcpy(message, "Odhlásenie užívateľa úspešné\n");
    } else {
        strcpy(message, "Odhlásenie užívateľa neúspešné\n");
    }
    printf("%s", message);  // Výpis stavu do konzoly servera
    *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
    if (*n < 0)
        perror("Error writing to socket");
}

void deleteUser(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser) {
    bool logoutSuccess = false;
    char message[256];

    // Najskôr odhlásenie užívateľa
    pthread_mutex_lock(threadData->mutexLogin);
    for (int i = 0; i < loggedInCount; ++i) {
        if (strcmp(loggedUser->nickname, clientData->loggedInUsers[i]->nickname) == 0) {    // Ak je užívateľ ešte prihlásený, odhlásim ho
            // vymením ho s posledným a vynulujem
            if (i != loggedInCount - 1) {
                User *tempUser = clientData->loggedInUsers[i];
                clientData->loggedInUsers[i] = clientData->loggedInUsers[loggedInCount - 1];
                clientData->loggedInUsers[loggedInCount - 1] = tempUser;
            }
            clientData->loggedInUsers[loggedInCount - 1] = NULL;
            loggedInCount--;
            logoutSuccess = true;
            break;
        }
    }
    pthread_mutex_unlock(threadData->mutexLogin);

    // následne zmazanie užívateľa
    if (logoutSuccess) {
        bool deleteSuccess = false;

        pthread_mutex_lock(threadData->mutexRegister);
        for (int i = 0; i < registeredCount; ++i) {
            if (strcmp(loggedUser->nickname, clientData->registeredUsers[i]->nickname) == 0) {    // Ak je užívateľ registrovaný, zmažem ho
                // vymením ho s posledným a vynulujem
//                if (i != registeredCount - 1) {
                    User *tempUser = clientData->registeredUsers[i];
                    clientData->registeredUsers[i] = clientData->registeredUsers[registeredCount - 1];
                    clientData->registeredUsers[registeredCount - 1] = tempUser;
//                }
                registeredCount--;

                // Uvoľnenie pamäte
                free(clientData->registeredUsers[registeredCount]->nickname);
                free(clientData->registeredUsers[registeredCount]->password);
                free(clientData->registeredUsers[registeredCount]->friends);
                pthread_mutex_unlock(threadData->mutexRegister);

                // Upratanie dát
                pthread_mutex_lock(threadData->mutexMessages);
                for (int j = 0; j < loggedUser->messagesCount; ++j) {
                    free(loggedUser->messages[j]->messageText);
                    free(loggedUser->messages[j]->sentFrom);
                    free(loggedUser->messages[j]);
                }
                free(loggedUser->messages);
                loggedUser->messagesCount = 0;
                pthread_mutex_unlock(threadData->mutexMessages);
                // Upratanie žiadostí o priateľstvo
                pthread_mutex_lock(threadData->mutexAddFriendRequest);
                for (int j = 0; j < loggedUser->addFriendRequestCount; ++j) {
                    free(loggedUser->addFriendRequest[j]);
                }
                free(loggedUser->addFriendRequest);

                pthread_mutex_unlock(threadData->mutexAddFriendRequest);
                // Upratanie žiadostí o zrušenie priateľstva
                pthread_mutex_lock(threadData->mutexDeleteFriendRequest);
                for (int j = 0; j < loggedUser->deleteFriendRequestCount; ++j) {
                    free(loggedUser->deleteFriendRequest[j]);
                }
                free(loggedUser->deleteFriendRequest);
                pthread_mutex_unlock(threadData->mutexDeleteFriendRequest);

                pthread_mutex_lock(threadData->mutexRegister);
                free(clientData->registeredUsers[registeredCount]);

                deleteSuccess = true;
                break;
            }
        }
        pthread_mutex_unlock(threadData->mutexRegister);
        // Ak úspešné zmazanie, vlož do message prvú hlášku, ináč vlož druhú hlášku
        strcpy(message, deleteSuccess ? "Zmazanie užívateľa úspešné\n" : "Zmazanie užívateľa neúspešné\n");
    } else strcpy(message, "Zmazanie užívateľa neúspešné - neúspešné odhlásenie\n");

    // Výpis stavu
    printf("%s", message);  // Výpis stavu do konzoly servera
    *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
    if (*n < 0)
        perror("Error writing to socket");

}

void addFriend(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser) {
    char message[256];    // Vyčistenie pomocného buffera
    char buffer[256];     // Vyčistenie pomocného buffera
    pthread_mutex_lock(threadData->mutexAddFriendRequest);
    sprintf(message, "%d", loggedUser->addFriendRequestCount);
    pthread_mutex_unlock(threadData->mutexAddFriendRequest);
    usleep(200000);

    printf("Requests: %s\n", message);
    *n = write(clientData->socketID, message, strlen(message) + 1);
    // Po zaslaní počtu žiadostí počkám pol sekundy
    usleep(200000);

    // Spracovanie žiadostí o priateľstvo
    char *requestingFriend;            // budem potrebovať aj mimo cyklu
    int requestsAccepted = 0;
    pthread_mutex_lock(threadData->mutexAddFriendRequest);
    for (int i = 0; i < loggedUser->addFriendRequestCount; ++i) {
        requestingFriend = loggedUser->addFriendRequest[0];
        if (requestingFriend != NULL) {
            pthread_mutex_lock(threadData->mutexRegister);
            User *requestingUser = findUser(requestingFriend, clientData->registeredUsers);
            pthread_mutex_unlock(threadData->mutexRegister);

            if (requestingUser != NULL) {
                // Zaslanie nicku žiadajúceho o priateľstvo klientovi
                bzero(message, 256);
                sprintf(message, "%s", requestingUser->nickname);
                *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
                // Prijatie odpovede
                *n = read(clientData->socketID, message, 255);   //  Načítame správu od klienta cez socket do buffra.
                if (strcasecmp(message, "Y") == 0) {                    // Ak potvrdil pridanie
                    pthread_mutex_lock(threadData->mutexLogin);
                    loggedUser->friends[loggedUser->friendsCount] = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);     // Alokujem priestor v zozname priateľov na konci
                    strcpy(loggedUser->friends[loggedUser->friendsCount], requestingUser->nickname);           // Pridám žiadajúceho do zoznamu užívateľov na koniec
                    loggedUser->friendsCount++;

                    // Zmazanie žiadosti po úspešnom accepte - najskôr výmena s poslednou, potom zmazanie poslednej
                    char *tempUser = loggedUser->addFriendRequest[0];
                    loggedUser->addFriendRequest[0] = loggedUser->addFriendRequest[loggedUser->addFriendRequestCount - 1];
                    loggedUser->addFriendRequest[loggedUser->addFriendRequestCount - 1] = tempUser;
                    loggedUser->addFriendRequest[loggedUser->addFriendRequestCount - 1] = NULL;
                    free(loggedUser->addFriendRequest[loggedUser->addFriendRequestCount - 1]);
                    free(tempUser);

                    requestsAccepted++;

                    // Pridanie užívateľa do priateľov žiadajúceho užívateľa
                    requestingUser->friends[requestingUser->friendsCount] = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);
                    strcpy(requestingUser->friends[requestingUser->friendsCount], loggedUser->nickname);     // Pridanie užívateľa na koniec jeho zoznamu
                    requestingUser->friendsCount++;
                    pthread_mutex_unlock(threadData->mutexLogin);
//                    usleep(200000);
                    sprintf(message, "Užívateľ úspešne pridaný");
                    printf("Užívateľ %s úspešne pridaný do zoznamu priateľov užívateľa %s. Počet priateľov: %d\n", requestingUser->nickname, loggedUser->nickname, loggedUser->friendsCount);
                    *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
                    usleep(200000);
                }
            }
        }
    }
    loggedUser->addFriendRequestCount -= requestsAccepted;
    pthread_mutex_unlock(threadData->mutexAddFriendRequest);

    // *** Pridanie užívateľa do priateľov
    // Zaslanie zoznamu všetkých užívateľov pre pridanie
    usleep(200000);     // sleep aby neposielalo moc rýchlo (a klient ich nezachytil)

    bzero(message, 256);
    pthread_mutex_lock(threadData->mutexRegister);          // Lock mutexu pre prípad (od)registrovania
    sprintf(message, "%d", registeredCount - 1);        // Počet registrovaných užívateľov - žiadajúci
    pthread_mutex_unlock(threadData->mutexRegister);
    printf("Posielam užívateľovi %s počet užívateľov na pridanie (%s)\n", loggedUser->nickname, message);
    *n = write(clientData->socketID, message, strlen(message)+1);
    usleep(200000);     // sleep aby neposielalo moc rýchlo (a klient ich nezachytil)

    pthread_mutex_lock(threadData->mutexRegister);      // Lock mutexu pre prípad (od)registrovania
    for (int i = 0; i < registeredCount; ++i) {
        char* user = clientData->registeredUsers[i]->nickname;
        if (strcmp(loggedUser->nickname, clientData->registeredUsers[i]->nickname) != 0) {      // Aby som neposlal samého seba
            bzero(message, 256);
            sprintf(message, "%s", clientData->registeredUsers[i]->nickname);
            *n = write(clientData->socketID, message, strlen(message)+1);       //  Pošleme odpoveď klientovi.
            usleep(200000);     // sleep aby neposielalo moc rýchlo (a klient ich nezachytil)
        }
    }
    pthread_mutex_unlock(threadData->mutexRegister);      // Lock mutexu pre prípad (od)registrovania
    printf("Všetky nicky poslané\n");
    // Samotné poslenie žiadosti - Výber nicku zo zoznamu
    *n = read(clientData->socketID, buffer, 255);

    char *requestingUser = strtok(buffer, "|");
    char *requestedUser = strtok(NULL, "|");

    printf("Užívateľ %s si chce pridať užívateľa %s\n", requestingUser, requestedUser);

    pthread_mutex_lock(threadData->mutexRegister);
    User *requested = findUser(requestedUser, clientData->registeredUsers);
    pthread_mutex_unlock(threadData->mutexRegister);

    if (requested != NULL) {    // AK som našiel hľadaný nick
        bool success = true;
        if (strcmp(requested->nickname, loggedUser->nickname) == 0) {       // Ak sa snažím pridať samého seba
            printf("Užívateľ %s sa pokúsil pridať samého seba\n", loggedUser->nickname);
            strcpy(message, "Nemôžete si pridať samého seba!\n");
            *n = write(clientData->socketID, message, strlen(message)+1);
            success = false;
        } else {
            // Ak sa snažím pridať užívateľa, ktorého už mám
            pthread_mutex_lock(threadData->mutexLogin);
            for (int i = 0; i < loggedUser->friendsCount; ++i) {
                if (strcmp(requested->nickname, loggedUser->friends[i]) == 0) {
                    sprintf(message, "Užívateľa %s už máte v priateľoch.\n", requested->nickname);
                    *n = write(clientData->socketID, message, strlen(message)+1);
                    printf("Užívateľ %s už má užívateľa %s v priateľoch.\n", loggedUser->nickname, requested->nickname);
                    success = false;
                    break;
                }
            }

            // Ak som danému užívateľovi žiadosť už poslal
            for (int i = 0; i < requested->addFriendRequestCount; ++i) {
                if (strcmp(loggedUser->nickname, requested->addFriendRequest[i]) == 0) {
                    sprintf(message, "Užívateľovi %s ste už žiadosť poslali.\n", requested->nickname);
                    *n = write(clientData->socketID, message, strlen(message)+1);
                    printf("Užívateľ %s už poslal žiadosť užívateľovi %s.\n", loggedUser->nickname, requested->nickname);
                    success = false;
                    break;
                }
            }
            pthread_mutex_unlock(threadData->mutexLogin);

            if(success) {
                // Ak sa nevyskytla žiadna chyba (priateľa ešte nemám v priateľoch), pošlem mu žiadosť
                pthread_mutex_lock(threadData->mutexAddFriendRequest);
                requested->addFriendRequest[requested->addFriendRequestCount] = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);     // Alokujem mu pristor
                strcpy(requested->addFriendRequest[requested->addFriendRequestCount++], loggedUser->nickname);
                pthread_mutex_unlock(threadData->mutexAddFriendRequest);
                sprintf(message, "Žiadosť úspešne poslaná\n");
                printf(GREEN"Užívateľovi %s %s"RESET, requested->nickname, message);
                *n = write(clientData->socketID, message, strlen(message)+1);
            }

        }
    } else {    // AK som nenašiel hľadaný nick
        sprintf(message, "Užívateľ s nickom %s nenájdený!\n", requestedUser);
        printf(RED"%s"RESET, message);
        *n = write(clientData->socketID, message, strlen(message)+1);
    }
}

void cancelFriend(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser) {
char buffer[256];
char message[256];
    // Žiadosti o zrušenie priateľstva
    printf(YELLOW"Posielam užívateľovi %s žiadosti o zrušenie priateľstva (počet: %d).\n"RESET, loggedUser->nickname, loggedUser->deleteFriendRequestCount);
    bzero(message,256);
    sprintf(message, "%d", loggedUser->deleteFriendRequestCount);
    *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
    usleep(200000);
    // *** Spracovanie žiadostí o zrušenie priateľstva
    // Prejdem všetky žiadosti
    int deleteFriendRequestCount = 0;
    int friendsCount = 0;
    char *requestingFriend;
    pthread_mutex_lock(threadData->mutexDeleteFriendRequest);
    for (int i = 0; i < loggedUser->deleteFriendRequestCount; ++i) {
        pthread_mutex_lock(threadData->mutexRegister);
        requestingFriend = loggedUser->deleteFriendRequest[0];
        User *requestingUser = findUser(requestingFriend, clientData->registeredUsers);
        pthread_mutex_unlock(threadData->mutexRegister);

        if (requestingUser != NULL) {                             // Ak užívateľ, ktorý poslal žiadosť existuje
            bzero(message, 256);
            sprintf(message, "%s", requestingUser->nickname);
            *n = write(clientData->socketID, message, strlen(message) + 1);           //  Pošleme odpoveď klientovi.
            // Prijatie odpovede
            printf("Čakám na odpoveď Y/N na žiadosť o zrušenie priateľa %s...", message);
            *n = read(clientData->socketID, message, 255);   //  Načítame správu od klienta cez socket do buffra.

            if (strcasecmp(message, "Y") == 0) {                    // Ak potvrdil zrušenie
                printf("odpoveď: \"%s\", odstraňujem...\n");
                // Výmena žiadosti s poslednou a násladné zmazanie poslednej
                char *tempUser = loggedUser->deleteFriendRequest[0];
                loggedUser->deleteFriendRequest[0] = loggedUser->deleteFriendRequest[loggedUser->deleteFriendRequestCount - 1];
                loggedUser->deleteFriendRequest[loggedUser->deleteFriendRequestCount - 1] = tempUser;
                loggedUser->deleteFriendRequest[loggedUser->deleteFriendRequestCount - 1] = NULL;
                free(loggedUser->deleteFriendRequest[loggedUser->deleteFriendRequestCount - 1]);
                free(tempUser);

                deleteFriendRequestCount++;

                // Zmazanie z priateľov
                for (int j = 0; j < loggedUser->friendsCount; ++j) {
                    if (loggedUser->friends[0] != NULL) {
                        if (strcmp(loggedUser->friends[0], requestingUser->nickname) == 0) {
//                          // Výmena priateľa s posledným a zmazanie posledného
                            char *tempUserFriend = loggedUser->friends[0];
                            loggedUser->friends[0] = loggedUser->friends[loggedUser->friendsCount - 1];
                            loggedUser->friends[loggedUser->friendsCount - 1] = tempUserFriend;
                            loggedUser->friends[loggedUser->friendsCount - 1] = NULL;
                            free(loggedUser->friends[loggedUser->friendsCount - 1]);
                            free(tempUserFriend);

                            friendsCount++;
                        }
                    }
                }
                printf("Užívateľ úspešne zmazaný\n");
            }
        }
    }
    loggedUser->deleteFriendRequestCount -= deleteFriendRequestCount;
    pthread_mutex_unlock(threadData->mutexDeleteFriendRequest);
    loggedUser->friendsCount -= friendsCount;
    printf("Všetky žiadosti vybavené\n");
    // *** Zmazanie priateľa
    // Zaslanie zoznamu priateľov
    printf("Posielam užívateľovi %s počet priateľov (%d)\n", loggedUser->nickname, loggedUser->friendsCount);
    usleep(200000);

    bzero(message, 256);
    sprintf(message, "%d", loggedUser->friendsCount);
    *n = write(clientData->socketID, message, strlen(message)+1);
    usleep(200000);
    // Poslatie nicku
    for (int i = 0; i < loggedUser->friendsCount; ++i) {
        char *user = loggedUser->friends[i];
        sprintf(message, "%s", loggedUser->friends[i]);
        *n = write(clientData->socketID, message, strlen(message)+1);
        usleep(200000);
    }
    printf("Všetky nicky odoslané\n");
    // Samotné poslanie žiadosti - Výber nicku zo zoznamu
    *n = read(clientData->socketID, buffer, 255);
    char *requestedUser = strtok(buffer, "|");

    printf("Užívateľ %s si chce odstrániť užívateľa %s zo zoznamu priateľov.\n", loggedUser->nickname, requestedUser);

    pthread_mutex_lock(threadData->mutexRegister);
    User *requested = findUser(requestedUser, clientData->registeredUsers);
    pthread_mutex_unlock(threadData->mutexRegister);

    if (requested != NULL) {        // Ak som našiel hľadaný nick
        if (strcmp(requested->nickname, loggedUser->nickname) == 0) {       // Ak sa snažím odstrániť samého seba
            printf("Užívateľ %s sa pokúsil odstrániť samého seba z priateľov\n", loggedUser->nickname);
            strcpy(message, "Nemôžete si odstrániť samého seba!");
            *n = write(clientData->socketID, message, strlen(message) + 1);
        } else {
            // Vytvorím žiadosť o zrušenie priateľstva konkrétnemu užívateľovi
            pthread_mutex_lock(threadData->mutexDeleteFriendRequest);
            requested->deleteFriendRequest[requested->deleteFriendRequestCount] = malloc(sizeof (char) * MAX_NICKNAME_LENGTH);  // Alokujem si priestor na konci zoznamu žiadostí
            strcpy(requested->deleteFriendRequest[requested->deleteFriendRequestCount], loggedUser->nickname);
            requested->deleteFriendRequestCount++;
            pthread_mutex_unlock(threadData->mutexDeleteFriendRequest);
            usleep(200000);
            printf("Žiadosť o zrušenie priateľstva užívateľa %s úspešne poslaná\n", requested->nickname);
            strcpy(message, "Žiadosť o zrušenie úspešne poslaná");
            *n = write(clientData->socketID, message, strlen(message)+1);
            printf("Logged user: %s\n", loggedUser->nickname);
            // Zruším priateľstvo u seba
            for (int i = 0; i < loggedUser->friendsCount; ++i) {
                if (strcmp(requested->nickname, loggedUser->friends[i]) == 0) {
                    char *tempUser = loggedUser->friends[i];
                    loggedUser->friends[i] = loggedUser->friends[loggedUser->friendsCount - 1];
                    loggedUser->friends[loggedUser->friendsCount - 1] = tempUser;
                    loggedUser->friends[loggedUser->friendsCount - 1] = NULL;
                    free(loggedUser->friends[loggedUser->friendsCount - 1]);
                    free(tempUser);
                    loggedUser->friendsCount--;
                    usleep(200000);
                    printf("Priateľ %s úspešne odstránený. Aktuálny počet priateľov: %d.\n", requested->nickname, loggedUser->friendsCount);
                    sprintf(message, "Priateľ úspešne odstránený. Aktuálny počet priateľov: %d", loggedUser->friendsCount);
                    *n = write(clientData->socketID, message, strlen(message)+1);
                }
            }
        }
    } else {                        // Ak som nenašiel hľadaný nick
        printf("Užívateľ s nickom %s nenájdený!\n", requestedUser);
        sprintf(message, "Užívateľ nenájdený!");
        *n = write(clientData->socketID, message, strlen(message)+1);
    }
}

void messages(int *n, ThreadData *threadData, ClientData *clientData, User *loggedUser) {
char message[256];
char buffer[256];
    // Zaslanie počtu správ
    printf(YELLOW"Posielam užívateľovi %s počet správ (%d).\n"RESET, loggedUser->nickname, loggedUser->messagesCount);
    bzero(message,256);
    sprintf(message, "Počet správ: %d", loggedUser->messagesCount);
    *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
    usleep(200000);
    // Poslanie všetkých správ v cykle
    for (int i = 0; i < loggedUser->messagesCount; ++i) {
        bzero(message, 256);
        printf("Posielam správu %d z %d\n", i+1, loggedUser->messagesCount);
        sprintf(message, "%s|%s", loggedUser->messages[i]->sentFrom, loggedUser->messages[i]->messageText);
        *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.


        *n = read(clientData->socketID, buffer, 255);
        if (i+1 != loggedUser->messagesCount)
            printf("Odosielam Ďalšiu správu.\n");
    }
    printf("Všetky správy zaslané\n");
    // *** Poslanie novej správy
    bzero(message, 256);

    bzero(buffer,256);
    (*n) = read(clientData->socketID, buffer, 255);
    if (strcasecmp(buffer, "SHOW") == 0) {
        printf("Užívateľ požiadal o zoznam priateľov\n");
        sprintf(message, "%d", loggedUser->friendsCount);   // pošlem počet
        *n = write(clientData->socketID, message, strlen(message)+1);

        for (int i = 0; i < loggedUser->friendsCount; ++i) {
            usleep(200000);
            bzero(message, 256);
            sprintf(message, "%s", loggedUser->friends[i]);     // pošlem nick
            *n = write(clientData->socketID, message, strlen(message)+1);
        }

        bzero(buffer,256);
        (*n) = read(clientData->socketID, buffer, 255);
    }
    printf("Čakám na nick\n");
    char *fromNick = strtok(buffer, "|");                // prijimatel
    char *messageText = strtok(NULL, "|");              // Text správy

    pthread_mutex_lock(threadData->mutexRegister);
    User *requested = findUser(fromNick, clientData->registeredUsers);
    pthread_mutex_unlock(threadData->mutexRegister);
    printf("Prijatá správa od užívateľa %s pre užívateľa %s.\n", loggedUser->nickname, fromNick);
    printf("Adresát: %s, | Text správy: %s\n", fromNick, messageText);

    if (requested != NULL) {
        Message *newMessage = malloc(sizeof(Message));      // Alokujem si priestor pre správu
        newMessage->sentFrom = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);
        strcpy(newMessage->sentFrom, loggedUser->nickname);

        newMessage->messageText = malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
        strcpy(newMessage->messageText, messageText);

        // Pridanie správy do užívateľovho zoznamu správ
        pthread_mutex_lock(threadData->mutexMessages);
        requested->messages[requested->messagesCount++] = newMessage;
        pthread_mutex_unlock(threadData->mutexMessages);

        sprintf(message, "Správa odoslaná užívateľovi");
        *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
        printf(GREEN"%s %s\n"RESET, message, requested->nickname);
    } else {
        strcat(message, "Neplatný nick užívateľa!");
        *n = write(clientData->socketID, message, strlen(message)+1);           //  Pošleme odpoveď klientovi.
        printf(RED"%s\n"RESET, message);
    }
}

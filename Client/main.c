//
// Created by hudec on 1. 1. 2022.
//

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include "operations.h"
#include "../colors.h"

#define MAX_FRIENDS_COUNT 256

typedef struct FriendList FriendsList;

int main(int argc, char *argv[]) {
    int sockfd, n;                  // Deskriptor socketu
    struct sockaddr_in serv_addr;
    struct hostent* server;         // Adresa serveru, s ktorým chcem komunikovať

    char buffer[256];

    if (argc < 3)                   // Skontrolujeme či máme dostatok argumentov.
    {
        fprintf(stderr,BOLD RED"Usage: %s hostname port\n"RESET, argv[0]);
        return 1;
    }

    server = gethostbyname(argv[1]);    //  Použijeme funkciu "gethostbyname" na získanie informácii o počítači, ktorého hostname je v prvom argumente.
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));    //  Vynulujeme a zinicializujeme adresu, na ktorú sa budeme pripájať.
    serv_addr.sin_family = AF_INET;
    bcopy(                                                 // Nastavenie komunikácie
            (char*)server->h_addr,
            (char*)&serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);   //  Vytvoríme si socket v doméne AF_INET. (TCP)
    if (sockfd < 0)
    {
        perror("Error creating socket");
        return 3;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)    //  Pripojíme sa na zadanú sieťovú adresu.
    {
        perror("Error connecting to socket");
        return 4;
    }
    printf(GREEN " *** Socket and Bind OK\n" RESET);


    // TODO uvodne menu - prihlasenie/registracia/koniec
    // prihlasenie - do spravy v sockete dat na zaciatok 'P' a delimiter je |
    // po prihlaseni sa natiahne jeho 'uzivatel' zo servera na clienta
    // potom sa mu zobrazia ziadosti o priatelstvo(caka sa na vstup Y/N)
    // po ziadostiach spravy - format [od koho]: sprava
    // po spravach menu - napis spravu/pridaj priatela/zrus priatela/odhlas->uvod.menu
    // napis spravu - vypise sa mu zoznam priatelov, zada meno, ak je OK, zada spravu
    // pridaj priatela - zada nick, ak sa najde na serveri, OK
    // priatel sa prida aj na serveri(uz nakodene) aj do clientovho 'uzivatela'
    // zrus priatela - detto
    // odhlas - posle sa socket s 'O', na serveri sa odhlasi(uz nakodene)

    // TODO
    FriendList *friendslist = malloc(sizeof(FriendList));
    friendslist->friends = malloc(sizeof(char*) * MAX_FRIENDS_COUNT);

    int friendsCount = 0;

    bool ended = false;
    bool logged = false;
    while (!ended) {

        if (!logged) {
            char option[1];
            int optionInt = 0;
            // Zobrazím úvodné menu (login, registrácia, koniec programu)
            printf("Úvodné menu:\n");
            printf("\t1: Registrácia\n");
            printf("\t2: Prihlásenie\n");
            printf("\t3: Ukončenie programu\n");

            while (optionInt < 1 || optionInt > 3) {
                printf("Zadajte možnosť: ");

//                scanf("%c", option);
                fgets(option, 2, stdin);
                fflush(stdin);
                optionInt = strtol(option, NULL, 10);
            }

            switch (optionInt) {
                case 1:     // Registrácia
                    registerUser(sockfd);
                    break;
                case 2:     // Login
                    logged = loginUser(sockfd);
                    break;
                case 3:     // Koniec programu
                    ended = true;
                    n = write(sockfd, "F", 2);
                    printf("Ukončujem program.\n");
                    break;
                default:
                    printf("Neplatná možnosť.\n");
                    break;
            }

        }
        if (logged) {
            char option[1];
            int optionInt = 0;

            // Zobrazím úvodné menu dostupné po prihlásení (Pridať priateľa, odstrániť priateľa, odoslať správu, zrušiť účet, odhlásiť sa)
            printf("Menu:\n");
            printf("\t1. Pridať priateľa\n");
            printf("\t2. Odstrániť priateľa\n");
            printf("\t3. Poslať správu priateľovi\n");
            printf("\t4. Zrušiť účet\n");
            printf("\t5. Odhlásiť sa\n");

            while (optionInt < 1 || optionInt > 5) {
                printf("Zadajte možnosť: ");

                fflush(stdin);
//                scanf(" %c", option);
                fgets(option, 2, stdin);
                fflush(stdin);
                optionInt = strtol(option, NULL, 10);
            }
            printf("Zvolená moťnosť: %d\n", optionInt);
            switch (optionInt) {
                case 1:
                    addFriend(sockfd, friendslist);
                    break;
                case 2:
                    deleteFriend(sockfd, friendslist);
                    break;
                case 3:
                    sendMessage(sockfd, friendslist);
                    break;
                case 4:
                    if (deleteAccount(sockfd)) logged = false;
                    break;
                case 5:
                    if(logoutUser(sockfd)) logged = false;
                    break;
                default:
                    printf("Neplatná možnosť.\n");
                    break;
            }
        }
    }



    close(sockfd);      // Zatvoríme socket
    return 0;
}

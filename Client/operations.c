//
// Created by hudec on 1. 1. 2022.
//
#include <stdlib.h>
#include "operations.h"
#include "../colors.h"

char loggedUser[MAX_NICKNAME_LENGTH];

bool showRequests(int sockfd, FriendList *friendslist) {
    // Zobrazenie žiadostí
    char message[256];
    int n;
    bzero(message, 256);
    n = read(sockfd, message, 255);
    printf("%s", message);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    char *requests;
    requests = strtok(message, ": ");
    if (strcmp(requests, "Žiadosti") == 0) {
        int requestsCount = strtol(strtok(NULL, ": "), NULL, 10);
        printf("Počet žiadostí: %d\n", requestsCount);
        for (int i = 0; i < requestsCount; ++i) {
            bzero(message, 256);
            n = read(sockfd, message, 255);
            if (n < 0)
            {
                perror("Error reading from socket");
                return false;
            }
            printf("Máte žiadosť o priateľstvo od užívateľa %s.\n", message);

            printf("Chcete potvrdiť túto žiadosť? [Y/N]: ");
            char response;
            char option[2];
            scanf("%*c%c", &response);
            sprintf(option, "%c", response);
            n = write(sockfd, option, strlen(option)+1);
            if (n < 0)
            {
                perror("Error writing to socket");
                return false;
            }
            if (strcasecmp(option, "Y") == 0) {
                // Prijatie potvrdenia o pridaní
                char requestingUser[MAX_NICKNAME_LENGTH];
                strcpy(requestingUser, message);
                bzero(message, 256);
                n = read(sockfd, message, 255);
                if (n < 0)
                {
                    perror("Error reading from socket");
                    return false;
                }
                printf("%s\n", message);        // Malo by vypísať stav.
                friendslist->friends[friendslist->friendsCount] = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);
                strcpy(friendslist->friends[friendslist->friendsCount], requestingUser);
                friendslist->friendsCount++;
            }
        }

    } else {    // Žiadne nové ziadosti o priateľstvo
        printf("Žiadne nové žiadosti o priateľstvo.\n");
    }
    return true;
}

bool loginUser(int sockfd, FriendList *friendsList) {
    char login[MAX_NICKNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    printf("Zadajte prihlasovacie meno (max. %d znakov): ", MAX_NICKNAME_LENGTH);
    scanf("%s", login);
    printf("Zadajte heslo (max. %d znakov): ", MAX_PASSWORD_LENGTH);
    scanf("%s", password);

    // Príprava a poslanie prihlasovacích údajov na server
    char message[256];
    bzero(message,256);
    sprintf(message, "L|%s|%s", login, password);
    printf("Odoslaný reťazec: %s\n", message);
    int n = write(sockfd, message, strlen(message)+1);
    if (n < 0)
    {
        perror("Error writing to socket");
        return false;
    }
    // Potvrdenie/zamietnutie loginu
    bzero(message, 256);
    n = read(sockfd, message, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    // Potvrdenie servera
    if (strcmp(message, "Login prebehol úspešne\n") == 0) {
        strcpy(loggedUser, login);
        printf(GREEN"Prihlásenie úspešné"RESET", prihlásený užívateľ: %s.\n", login);
        return showRequests(sockfd, friendsList);
    } else {
        printf(RED"Prihlásenie neúspešné - nesprávne meno alebo heslo.\n"RESET);
        return false;
    }
}

bool registerUser(int sockfd) {
    char login[MAX_NICKNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    printf("Zadajte prihlasovacie meno (max. %d znakov): ", MAX_NICKNAME_LENGTH);
    scanf("%s", login);
    printf("Zadajte heslo (max. %d znakov): ", MAX_PASSWORD_LENGTH);
    scanf("%s", password);


    // Príprava a poslanie prihlasovacích údajov na server
    char message[256];
    bzero(message, 256);
    sprintf(message, "R|%s|%s", login, password);
    int n = write(sockfd, message, strlen(message)+1);
    if (n < 0)
    {
        perror("Error writing to socket");
        return false;
    }
    // Potvrdenie/zamietnutie loginu
    bzero(message, 256);
    n = read(sockfd, message, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    // Potvrdenie servera
    if (strcmp(message, "Registrácia užívateľa úspešná.\n") == 0) {
        printf("%s", message);
        return true;
    } else if (strcmp(message, "Užívateľ s daným menom už je zaregistrovaný!\n") == 0) {
        printf("%s", message);
        return false;
    } else {    // Iná prijatá chyba zo servera
        printf("%s", message);
        return false;
    }
}

bool addFriend(int sockfd, FriendList *friendslist) {
    fflush(stdin);
    char message[256];
    bzero(message, 256);
    sprintf(message, "A");
    int n = write(sockfd, message, strlen(message)+1);
    if (n < 0)
    {
        perror("Error writing to socket");
        return false;
    }
    // Prijatie počtu žiadostí
    printf("Posielam žiadosť o nové žiadosti\n");
    bzero(message, 256);
    n = read(sockfd, message, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    int requestsCount = strtol(message, NULL, 10);
    printf("Počet nových žiadostí: %d\n", requestsCount);
    // Spracovanie žiadostí:
    for (int i = 0; i < requestsCount; ++i) {
        bzero(message, 256);
        n = read(sockfd, message, 255);
        if (n < 0)
        {
            perror("Error reading from socket");
            return false;
        }
        char requestingUser[MAX_NICKNAME_LENGTH];
        strcpy(requestingUser, message);
        printf("Užívateľ %s Vás žiada o priateľstvo, chcete ho pridať? [Y/N]: ", message);
        // Potvrdenie žiadosti
        char temp;
        char option[256];
        scanf("%*c%c", &temp);
        sprintf(option,"%c", temp);
        printf("Načítal som a odosielam \"%s\"\n", option);
        n = write(sockfd, option, strlen(option)+1);
        if (n < 0)
        {
            perror("Error writing to socket");
            return false;
        }
        if (strcasecmp(option, "Y") == 0) {
            // Prijatie potvrdenia o pridaní
            bzero(message, 256);
            n = read(sockfd, message, 255);
            if (n < 0)
            {
                perror("Error reading from socket");
                return false;
            }
            printf("%s\n", message);
            friendslist->friends[friendslist->friendsCount] = malloc(sizeof(char) * MAX_NICKNAME_LENGTH);
            strcpy(friendslist->friends[friendslist->friendsCount], requestingUser);
            friendslist->friendsCount++;
        }
    }
    n = read(sockfd, message, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    int registeredCount = strtol(message, NULL, 10);
    printf("Počet registrovaných užívateľov: %d\n", registeredCount);
    // Výpis registrovaných užívateľov
    for (int i = 0; i < registeredCount; ++i) {
        bzero(message, 256);
        n = read(sockfd, message, 255);
        if (n < 0)
        {
            perror("Error reading from socket");
            return false;
        }
        printf("\tNick: %s\n", message);
    }
    // Samotné poslanie žiadosti:
    char requestedFriend[MAX_NICKNAME_LENGTH];
    printf("Zadaj nick užívateľa, ktorého si chceš pridať: ");
    scanf("%s", requestedFriend);
    bzero(message, 256);
    sprintf(message, "%s|%s", loggedUser, requestedFriend);
    n = write(sockfd, message, strlen(message)+1);
    if (n < 0)
    {
        perror("Error writing to socket");
        return false;
    }
    // Odpoveď zo servera na žiadosť:
    bzero(message, 256);
    n = read(sockfd, message, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    if (strcmp(message, "Žiadosť úspešne poslaná\n") == 0) {
        printf(GREEN"Žiadosť o priateľstvo úspešne poslaná užívateľovi %s\n"RESET, requestedFriend);
        return true;
    } else {
        printf(RED"Zaslanie žiadosti o priateľstvo užívateľovi %s zlyhalo. "RESET"Dôvod: %s\n", requestedFriend, message);
        return false;
    }
}

bool logoutUser(int sockfd) {
    char message[256];
    sprintf(message, "O|%s", loggedUser);
    int n = write(sockfd, message, strlen(message)+1);
    if (n < 0)
    {
        perror("Error writing to socket");
        return false;
    }
    // Potvrdenie odhlásenia zo servera
    bzero(message, 256);
    n = read(sockfd, message, 255);
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    if (strcmp(message, "Odhlásenie užívateľa úspešné\n") == 0) {
        printf("%s", message);
        return true;
    } else {
        printf("%s", message);
        return false;
    }
}

bool deleteAccount(int sockfd) {
    char message[256];
    printf("Naozaj chcete zrušiť svoj účet? [Y/N]: ");
    char option[2];
    char response;
    bzero(option, 1);
//        fflush(stdin);
    while ((getchar()) != '\n');
    scanf("%c", &response);
    sprintf(option, "%c", response);

////    fgets(option, 2, stdin);
////    fflush(stdin);
    printf(YELLOW"Option: %s\n"RESET, option);
    if (strcasecmp(option, "Y") == 0) {
        printf("Odosielam žiadosť o zrušenie na server...\n");
        sprintf(message, "D");
        int n = write(sockfd, message, strlen(message) + 1);
        if (n < 0) {
            perror("Error writing to socket");
            return false;
        }
        bzero(message, 256);
        n = read(sockfd, message, 255);
        if (n < 0) {
            perror("Error reading from socket");
            return false;
        }
        if (strcmp(message, "Zmazanie užívateľa úspešné\n") == 0) {
            printf(GREEN"%s"RESET, message);
            return true;
        } else {
            printf(RED"%s"RESET, message);
            return false;
        }
    }
    return false;
}

bool sendMessage(int sockfd, FriendList *friendsList) {
    char message[256];
    char receiver[MAX_NICKNAME_LENGTH];
    char messageText[MAX_MESSAGE_LENGTH];

    bzero(message, 256);
    sprintf(message, "M");
    int n = write(sockfd, message, strlen(message) + 1);

    // Prijatie počtu správ
    bzero(message, 256);
    n = read(sockfd, message, 255);
    strtok(message, ":");
    char *count = strtok(NULL, ":");
    int messagesCount = strtol(count, NULL, 10);
    printf("Aktuálny počet správ: %d\n", messagesCount);
    // Výpis prijatých správ:
    for (int i = 0; i < messagesCount; ++i) {
        bzero(message, 256);
        n = read(sockfd, message, 255);
        char *sender = strtok(message, "|");
        char *text = strtok(NULL, "|");

        // Dešifrovanie spravy Cezarovou sifrou
        for (int i = 0; text[i] != '\0'; ++i) {
            text[i] -= 1;
        }
        // Koniec dešifrovania

        printf("Odosielateľ: %s\nText správy: %s\n", sender, text);
        sleep(5);
        bzero(message, 256);
        strcpy(message, "NextMessage");
        if (i+1 != messagesCount)
            printf("Žiadam ďalšiu správu\n");
        n = write(sockfd, message, strlen(message)+1);
    }
    // Odoslanie novej správy
    bzero(message, 256);
    printf("Zadajte nick priateľa, ktorému chcete poslať správu\n alebo zadajte SHOW pre vyžiadanie zoznamu priateľov: ");
    scanf("%s", receiver);
    printf("\n");
    if (strcasecmp(receiver, "SHOW") == 0) {
        printf("Žiadam o zoznam priateľov\n");
        n = write(sockfd, receiver, strlen(receiver)+1);
        bzero(receiver, MAX_NICKNAME_LENGTH);
        n = read(sockfd, receiver, MAX_MESSAGE_LENGTH + 1);
        int friendsCount = strtol(receiver, NULL, 10);
        if (friendsCount > 0) {
            for (int i = 0; i < friendsCount; ++i) {
                bzero(receiver, MAX_NICKNAME_LENGTH);
                n = read(sockfd, receiver, MAX_MESSAGE_LENGTH + 1);
                printf("Nick: %s\n", receiver);
            }
        } else printf("Nemáte žiadnych priateľov :(\n");

        bzero(message, 256);
        printf("Zadajte nick priateľa, ktorému chcete poslať správu: ");
        scanf("%s", receiver);
        printf("\n");
    }
    // Samotné poslanie správy
    printf("Zadajte text správy (max. %d znakov): ", MAX_MESSAGE_LENGTH);
    while ((getchar()) != '\n');
    scanf("%[^\n]s", messageText);  // trim na NewLine namiesto whitespace

    // Zasifrovanie spravy Cezarovou sifrou
    for (int i = 0; messageText[i] != '\0'; ++i) {
        messageText[i] += 1;
    }
    // Koniec sifrovania

    sprintf(message, "%s|%s", receiver, messageText);

    n = write(sockfd, message, strlen(message)+1);

    // Prijatie potvrdenia o odoslaní
    bzero(message, 256);
    n = read(sockfd, message, 255);
    if (strcmp(message, "Správa odoslaná užívateľovi") == 0) {
        printf(GREEN"Správa úspešne odoslaná užívateľovi %s.\n"RESET, receiver);
        return true;
    } else {
        printf(RED"Odoslanie správy zlyhalo."RESET" Dôvod: %s\n", message);
        return false;
    }
}

bool deleteFriend(int sockfd, FriendList *friendsList) {
    char message[256];
    bzero(message, 256);
    sprintf(message, "C");
    int n = write(sockfd, message, strlen(message) + 1);
    // Prijatie počtu žiadostí o zrušenie
    bzero(message, 256);
    n = read(sockfd, message, 255);
    int deleteFriendCount = strtol(message, NULL, 10);
    printf("Počet žiadostí o zrušenie priateľstva: %d\n", deleteFriendCount);
    // Spracovanie žiadostí o zrušenie
    for (int i = 0; i < deleteFriendCount; ++i) {
        bzero(message, 256);
        n = read(sockfd, message, 255);
        if (n < 0)
        {
            perror("Error reading from socket");
            return false;
        }
        printf("Užívateľ %s Vás žiada zrušenie priateľstva, chcete ho zrušiť? [Y/N]: ", message);
        // Potvrdenie žiadosti
        char option[1];
        fflush(stdin);
        scanf("%s", option);
        n = write(sockfd, option, strlen(option)+1);
        if (n < 0)
        {
            perror("Error writing to socket");
            return false;
        }
        if (strcasecmp(option, "Y") == 0) {

            for (int j = 0; j < friendsList->friendsCount; ++j) {
                if (strcmp(friendsList->friends[i], message) == 0) {
                    char *tempUser = friendsList->friends[i];
                    friendsList->friends[i] = friendsList->friends[friendsList->friendsCount - 1];
                    friendsList->friends[friendsList->friendsCount - 1] = tempUser;
                    friendsList->friends[friendsList->friendsCount - 1] = NULL;
                    free(friendsList->friends[friendsList->friendsCount - 1]);
                    friendsList->friendsCount--;
                    break;
                }
            }
        }
    }   // Spracované všetky žiadosti
    printf("Všetky žiadosti vybavené\n");
    // Poslanie žiadosti o zrušenie
    bzero(message, 256);
    n = read(sockfd, message, 255);     // Prijatie počtu priateľov
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    int friendsCount = strtol(message, NULL, 10);
    printf("Počet priateľov: %d\n", friendsCount);
    for (int i = 0; i < friendsCount; ++i) {
        bzero(message, 256);
        n = read(sockfd, message, 255);     // Prijatie nicku
        if (n < 0)
        {
            perror("Error reading from socket");
            return false;
        }
        printf("\tNick: %s\n", message);
    }
    // Samotné poslanie žiadosti - Poslanie nicku
    bzero(message, 256);
    printf("Zadajte nick priateľa, ktorého chcete zmazať: ");
    scanf("%s", message);
    printf("\nOdosielam žiadosť...\n");

    n = write(sockfd, message, strlen(message)+1);      // Pošlem nick
    if (n < 0)
    {
        perror("Error writing to socket");
        return false;
    }
    // Prijatie odpovede zo servera
    bzero(message, 256);
    n = read(sockfd, message, 255);     // Prijatie nicku
    if (n < 0)
    {
        perror("Error reading from socket");
        return false;
    }
    if (strcmp(message, "Nemôžete si odstrániť samého seba!") == 0 ||
        strcmp(message, "Užívateľ nenájdený!") == 0) {
        printf(RED"%s\n"RESET, message);
        return false;
    } else if (strcmp(message, "Žiadosť o zrušenie úspešne poslaná") == 0) {
        printf(GREEN"%s\n"RESET, message);
        n = read(sockfd, message, 255);     // Prijatie nicku
        if (n < 0)
        {
            perror("Error reading from socket");
            return false;
        }
        printf("%s\n", message);

        return true;
    } else {
        printf("Iná chyba: %s\n", message);
        return false;
    }
}

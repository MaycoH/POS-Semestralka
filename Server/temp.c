int test() {
    // Po skončení komunikácie s klientom upratanie
    // Ak bol klient aj prihlásený, mal nejaké dáta, ktoré musím upratať
    if (loggedUser != NULL) {
        // Upratanie správ
        pthread_mutex_lock(thrData->mutexMessages);
        for (int i = 0; i < loggedUser->messagesCount; ++i) {
            free(loggedUser->messages[i]->messageText);
            free(loggedUser->messages[i]->sentFrom);
            free(loggedUser->messages[i]);
        }
        free(loggedUser->messages);
        loggedUser->messagesCount = 0;
        pthread_mutex_unlock(thrData->mutexMessages);
        // Upratanie žiadostí o priateľstvo
        pthread_mutex_lock(thrData->mutexAddFriendRequest);
        for (int i = 0; i < loggedUser->addFriendRequestCount; ++i) {
            free(loggedUser->addFriendRequest[i]);
        }
        free(loggedUser->addFriendRequest);
        pthread_mutex_unlock(thrData->mutexAddFriendRequest);
        // Upratanie žiadostí o zrušenie priateľstva
        pthread_mutex_lock(thrData->mutexDeleteFriendRequest);
        for (int i = 0; i < loggedUser->deleteFriendRequestCount; ++i) {
            free(loggedUser->deleteFriendRequest[i]);
        }
        free(loggedUser->deleteFriendRequest);
        pthread_mutex_unlock(thrData->mutexDeleteFriendRequest);



        requested->addFriendRequest[requested->addFriendRequestCount];
    }
}
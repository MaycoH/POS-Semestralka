// Created by hudec on 30. 12. 2021.
// Source: https://stackoverflow.com/questions/1961209/making-some-text-in-printf-appear-in-green-and-red

// Example:
// fprintf(stderr,BOLD RED"Usage: "RESET RED" %s "BOLD"PORT\n"RESET, argv[0]);
// perror(RED " *** Error binding socket address\n" RESET);
// printf(GREEN " *** Socket and Bind OK\n" RESET);

#ifndef SEMESTRALKA_COLORS_H
#define SEMESTRALKA_COLORS_H

#define RESET   "\033[0m"
#define BOLD    "\33[1m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */

#endif //SEMESTRALKA_COLORS_H

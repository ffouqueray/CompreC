#include <stdio.h>
#include "colors.h"

// Passage des printf dans la console en vert
void green() {
    printf("\033[0;32m");
}

// Passage des printf dans la console en bleu
void blue() {
    printf("\033[0;34m");
}

// Passage des printf dans la console en magenta
void magenta() {
    printf("\033[0;35m");
}

// Passage des printf dans la console en cyan
void cyan() {
    printf("\033[0;36m");
}

// Passage des printf dans la console en celle par defaut (le gris degueu la berk)
void reset () {
printf("\033[0m");
}
/* Test for header.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"

int main(int argc, char *argv[])
{
    int i;

    printf("PHDR_INDEX_T = %i\n", sizeof(peso_header));
    strcpy(peso_header[PHDR_OBSERVER_E].value, "Jan Fuchs");
    strcpy(peso_header[PHDR_OBSERVER_E].comment, "Observers");

    strcpy(peso_header[PHDR_OBSERVAT_E].value, "ONDREJOV");
    strcpy(peso_header[PHDR_OBSERVAT_E].comment,
            "Name of observatory (IRAF style)");

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        printf("%i. %s = %s / %s\n", i, peso_header[i].key,
                peso_header[i].value, peso_header[i].comment);
    }
}

#include <stdio.h>
#include <stdlib.h>

int main(){
        char *a = malloc(6);
        a[0] = 'h';
        a[1] = 'e';
        a[2] = 'y';
        a[3] = '\0';
        printf("In ymalloc.c - a[0] : %c, %p\n", a[0], a);
        return 0;
}


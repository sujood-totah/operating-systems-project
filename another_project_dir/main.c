#include <stdio.h>

int add_numbers(int a, int b){
    return a+b;
}

int main(void) {
    printf("Hello, World!\n");
    int a=1;
    int b=4;
    add_numbers(a,b);
    int i;
    return 0;
}
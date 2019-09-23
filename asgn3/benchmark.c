#include <stdio.h>  
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
double* arr = (double*)malloc(sizeof(double) * 536870912); 
double sum = 0.0;

for (int id=0; id<5000000; id++) {
    int iIndex = rand() % 536870912;
    sum += arr[iIndex];
}

exit(0);
}

// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NODE_H
#define NODE_H

// node struct
typedef struct node {
    char name[64];
    char type;
    struct node *parentPtr, *childPtr, *siblingPtr;
} NODE;

#endif
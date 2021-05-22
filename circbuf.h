#ifndef _CIRCBUF_H_
#define _CIRCBUF_H_


#include <stdio.h>
#include <stdlib.h>

typedef struct 
{
    int* head;
    int* tail;
    void* base;
    int*  length;
    int*  count;
    
} CB_t;

// Estructura a contener
typedef struct Pedido
{
    int estado;
    int nroPed;
    int tipoPed;
    int distPed;
} Pedido;

//  Declaración de funciones de manejo del buffer
void CB_init(CB_t**, int);
int  CB_remove(CB_t*, Pedido*);
int  CB_add(CB_t*, Pedido*);
int  CB_isEmpty(CB_t*);
int  CB_isFull(CB_t*);

#endif
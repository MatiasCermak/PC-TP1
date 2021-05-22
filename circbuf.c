/* Implementación de buffer circular creada por Matías Cermak para la materia Programación Concurrente
  de la Universidad Blas Pascal*/

#include "circbuf.h"
// Implementación de funciones del buffer

void CB_init(CB_t** cbuf, int size){
    *cbuf = (CB_t*) malloc(sizeof(CB_t));
    void *aux = calloc(size, sizeof(Pedido));
    (*cbuf)->head = calloc(1, sizeof(int));
    (*cbuf)->tail = calloc(1, sizeof(int));
    (*cbuf)->base = aux;
    (*cbuf)->length = calloc(1, sizeof(int));
    *(*cbuf)->length = size;
    (*cbuf)->count = calloc(1, sizeof(int));
}

int CB_remove(CB_t* cbuf, Pedido* data){
    if(!cbuf || !cbuf->base || !cbuf->head || !cbuf->tail){
        return -1;
    }
    if( *cbuf->count == 0 ){
        return -1;
    }

    Pedido *aux = ((Pedido*)cbuf->base + *cbuf->tail);
    data->distPed = aux->distPed;
    data->estado = aux->estado;
    data->nroPed = aux->nroPed;
    data->tipoPed = aux->tipoPed;

    if(*cbuf->tail == (*cbuf->length - 1)){
        *cbuf->tail = 0;
        *(cbuf->count)-=1; 
    }
    else{
        *(cbuf->tail)+=1;
        *(cbuf->count)-=1;
    }
    return 0;
}

int CB_add(CB_t* cbuf, Pedido* newAdd){
    if( !cbuf || !cbuf->base || !cbuf->head || !cbuf->tail ){
        return -1;
    }
    if( *cbuf->count == (*cbuf->length) ){
        return -1;
    }
    
    Pedido *aux = ((Pedido*)cbuf->base + *cbuf->head);
    aux->distPed = newAdd->distPed;
    aux->estado = newAdd->estado;
    aux->nroPed = newAdd->nroPed;
    aux->tipoPed = newAdd->tipoPed;

    if(*(cbuf->head) == (*(cbuf->length) - 1)){
        *(cbuf->head) = 0;
        *(cbuf->count)+=1; 
    }
    else{
        *(cbuf->head)+=1;
        *(cbuf->count)+=1; 
    }
    return 0;
}

int CB_isEmpty(CB_t* cbuf){
    if(*(cbuf->count) == 0) return 1;
    else return 0;
}

int CB_isFull(CB_t* cbuf){
    if(*(cbuf->count) == *(cbuf->length)) return 1;
    else return 0;
}

/*
int main(){
    CB_t* cb;
    CB_init(&cb, 4);
    int counter = 0;
    Pedido *pedActual;
    while (counter != 5)
    {
        pedActual = (Pedido *)calloc(1, sizeof(Pedido));
        pedActual->distPed = (rand() % 5) + 1;
        pedActual->estado = 0;
        pedActual->nroPed = counter;
        pedActual->tipoPed = (rand() % 5) + 1;
        printf("%i\n", CB_add(cb,pedActual));
        counter++;
    }
    counter = 0;
    while (counter != 5)
    {
        printf("%i", CB_remove(cb,pedActual));
        printf("%i", pedActual->nroPed);
        counter++;
    }
}*/
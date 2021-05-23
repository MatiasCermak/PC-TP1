#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include "MonitoresSimple.h"

struct Monitor_t* CrearMonitor  () {
  int error=0;
  struct Monitor_t *aux=NULL;

  aux = (struct Monitor_t*)(calloc(1, sizeof(struct Monitor_t)));

  if (aux != NULL) {
    aux->readyEmpty = 1;
    aux->readyFull = 0;
    CB_init(&(aux->cbuf), 8);
    error += pthread_cond_init(&aux->condEmpty, NULL);
    if (error)
      perror("pthread_cond_init()");

    error += pthread_cond_init(&aux->condFull, NULL);
    if (error)
      perror("pthread_cond_init()");

    error += pthread_mutex_init(&aux->leer, NULL);
    if (error)
      perror("pthread_mutex_init()");

    error += pthread_mutex_init(&aux->escribir, NULL);
    if (error)
      perror("pthread_mutex_init()");

    error += pthread_mutex_init(&aux->contadorBuffer, NULL);
    if (error)
      perror("pthread_mutex_init()");

    pthread_cond_broadcast(&aux->condFull);
    pthread_mutex_lock(&aux->leer);
    pthread_mutex_unlock(&aux->escribir);
    pthread_mutex_unlock(&aux->contadorBuffer);
  }

  return aux;
}

int GuardarDato (struct Monitor_t *m, Pedido* dato) {
  int error=0;
  error = pthread_mutex_lock(&m->escribir);
  if (error)
    perror("pthread_mutex_lock()");
  else
    while (m->readyFull){
      error = pthread_cond_wait(&m->condFull, &m->escribir);
    }
  if (error)
    perror("pthread_cond_wait()");
  else {
    error = CB_add(m->cbuf, dato, &m->contadorBuffer);
    if (error)
      perror("CB_add()");
    pthread_mutex_lock(&m->contadorBuffer);
    m->readyFull = CB_isFull(m->cbuf);
    pthread_mutex_unlock(&m->contadorBuffer);
    m->readyEmpty = 0;
    pthread_cond_signal(&m->condFull);
    pthread_cond_signal(&m->condEmpty);
  }

  if (error)
    perror("pthread_cond_signal()");
  else{
    error = pthread_mutex_unlock(&m->escribir);
    error = pthread_mutex_unlock(&m->leer);
  }
  if (error)
    perror("pthread_mutex_unlock()");

  return error;
 }


int LeerDato (struct Monitor_t *m, Pedido* dato) {
  int error=0;

  error = pthread_mutex_lock(&m->leer);
  if (error)
    perror("pthread_mutex_lock()");
  else
    while (m->readyEmpty){
      error = pthread_cond_wait(&m->condEmpty, &m->leer);
    }
  if (error)
    perror("pthread_cond_wait()");
  else {
    error = CB_remove(m->cbuf, dato, &m->contadorBuffer);
    if(error){
      perror("CB_remove()");
    }
    pthread_mutex_lock(&m->contadorBuffer);
    m->readyEmpty = CB_isEmpty(m->cbuf);
    pthread_mutex_unlock(&m->contadorBuffer);
    m->readyFull = 0;
    pthread_cond_signal(&m->condFull);
    pthread_cond_signal(&m->condEmpty);
  }

  if (error)
    perror("pthread_cond_signal()");
  else
    error = pthread_mutex_unlock(&m->leer);

  if (error)
    perror("pthread_mutex_unlock()");


  return error;
 }


void BorrarMonitor (struct Monitor_t *m) {
  if (m != NULL) {
    pthread_cond_destroy(&m->condFull);
    pthread_cond_destroy(&m->condEmpty);
    pthread_mutex_destroy(&m->leer);
    pthread_mutex_destroy(&m->escribir);
    pthread_mutex_destroy(&m->contadorBuffer);
    free(m);
  }
}

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

int CB_remove(CB_t* cbuf, Pedido* data, pthread_mutex_t* cont){
    if(!cbuf || !cbuf->base || !cbuf->head || !cbuf->tail){
        return -1;
    }
    if( *cbuf->count == 0 ){
        return -1;
    }

    pthread_mutex_lock(cont);
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
    pthread_mutex_unlock(cont);
    return 0;
}

int CB_add(CB_t* cbuf, Pedido* newAdd, pthread_mutex_t *cont){
    if( !cbuf || !cbuf->base || !cbuf->head || !cbuf->tail ){
        return -1;
    }
    if( *cbuf->count == (*cbuf->length) ){
        return -1;
    }
    
    pthread_mutex_lock(cont);
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
    pthread_mutex_unlock(cont);
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
void * threadLect(void *);
void * threadWrt(void *);

int main(){
  struct Monitor_t* m = CrearMonitor();

  pthread_t threadl[7];
  pthread_t threadw[7];
  for (int i = 0; i < 7; i++)
  {

    pthread_create(&threadl[i],  NULL, threadLect, m);
    pthread_create(&threadw[i],  NULL, threadWrt, m);

  }
  for (int i = 0; i < 7; i++)
  {
    pthread_join(threadl[i],  NULL);
    pthread_join(threadw[i],  NULL);
  }

}

void* threadLect(void * tmp){
  struct Monitor_t* m = (struct Monitor_t*)tmp;
  int dato;
  for (int i = 0; i < 20; i++)
  {
    LeerDato(m, &dato);
    printf("%i\n", dato);
  }
  pthread_exit(NULL);
}

void* threadWrt(void * tmp){
  struct Monitor_t* m = (struct Monitor_t*)tmp;

  for (int i = 0; i < 20; i++)
  {
    GuardarDato(m, i);
  }
  pthread_exit(NULL);
}*/
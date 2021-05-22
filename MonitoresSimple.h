#ifndef _MONITORSIMPLE_H_
#define _MONITORSIMPLE_H_

// Estructura del buffer circular
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

struct Monitor_t {
  int readyFull;
  int readyEmpty;
  CB_t* cbuf;
  pthread_cond_t  condEmpty, condFull;
  pthread_mutex_t escribir, leer, contadorBuffer;
};

//  Declaración de funciones de manejo del buffer
void CB_init(CB_t**, int);
int  CB_remove(CB_t*, Pedido*, pthread_mutex_t *);
int  CB_add(CB_t*, Pedido*, pthread_mutex_t *);
int  CB_isEmpty(CB_t*);
int  CB_isFull(CB_t*);

// Declaración de funciones del monitor
struct Monitor_t* CrearMonitor();
int        GuardarDato   (struct Monitor_t *m, Pedido* dato);
int        LeerDato      (struct Monitor_t *m, Pedido* dato);
void       BorrarMonitor (struct Monitor_t *m);

#endif

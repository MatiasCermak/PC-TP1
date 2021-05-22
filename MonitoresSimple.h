#ifndef _MONITORSIMPLE_H_
#define _MONITORSIMPLE_H_

#include "circbuf.h"

struct Monitor_t {
  int readyFull;
  int readyEmpty;
  CB_t* cbuf;
  pthread_cond_t  condEmpty, condFull;
  pthread_mutex_t escribir, leer;
};


struct Monitor_t* CrearMonitor  ();
int        GuardarDato   (struct Monitor_t *m, Pedido* dato);
int        LeerDato      (struct Monitor_t *m, Pedido* dato);
void       BorrarMonitor (struct Monitor_t *m);

#endif

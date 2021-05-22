#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "MonitoresSimple.h"
#include "circbuf.h"

int isFin = 0; //Como no podemos pasar parametros a los signal handlers, nos vimos obligados a crear una variable global

//Si bien el encargado se ejecuta desde el main, decidimos crear un struct encargado para mejor lectura del código
typedef struct
{
    pthread_mutex_t *mtxTel, *mtxEnc;
    Pedido *pedActual, *pedMemComp1, *pedMemComp2;
    struct Monitor_t *monEC; //Monitor encargado-cocinero
    sem_t *semW1, *semR1, *semW2, *semR2;
} Encargado;

typedef struct
{
    pthread_mutex_t *mtxTel;
    pthread_mutex_t *mtxEnc;
    Pedido *pedActual;
    int *timer;
} Telefono;

typedef struct
{
    Pedido *pedActual;
    pthread_mutex_t *mtxVarLast;
    struct Monitor_t *monEC; //Monitor encargado-cocinero
    struct Monitor_t *monCD; //Monitor delivery-cocinero
    int *last;               //Se irá reduciendo hasta indicar cuál es el último cocinero, y este se encargará de eliminar los deliverys
} Cocinero;

typedef struct
{
    Pedido *pedActual, *pedMemComp;
    struct Monitor_t *monCD; //Monitor delivery-cocinero
    sem_t *semW;
    sem_t *semR;
} Delivery;

typedef struct
{
    Pedido **pedTel, **pedEnc, **pedCoc1, **pedCoc2, **pedCoc3, **pedDel1, **pedDel2;
} Output;

void *telefono(void *);
void *cocinero(void *);
void *delivery(void *);
void *printOutput(void *);
void *takeInput(void *);
void startTermination(int);
int mutexInit(pthread_mutex_t *, pthread_mutex_t *, pthread_mutex_t *);
int crearSemaforos(Encargado *);
int setTiempo();
int crearMemoriaCompartida(Encargado *, int *, int *);
int liberarMemoria(Encargado *, int *, int *);
int FuncionesDeEncargado(Encargado *, Telefono *, int *);

int main(int argc, char *argv[])
{
    //Declaración estructuras y variables
    int error = 0;
    int optionSelected = 0;
    int totalEarned = 0;
    pthread_t *threads;
    Encargado enc;
    Telefono tel;
    Cocinero coc1, coc2, coc3;
    Delivery del1, del2;
    Output out;

    //Asignación de pedidos a la variable out
    out.pedCoc1 = &coc1.pedActual;
    out.pedCoc2 = &coc2.pedActual;
    out.pedCoc3 = &coc3.pedActual;
    out.pedDel1 = &del1.pedActual;
    out.pedDel2 = &del2.pedActual;
    out.pedEnc = &enc.pedActual;
    out.pedTel = &tel.pedActual;

    //Creación de entero de cocineros
    int aux = 3;
    coc1.last = &aux;
    coc2.last = &aux;
    coc3.last = &aux;

    // Creación de semáforos y asignación.
    error = crearSemaforos(&enc);
    if (error != 0)
    {
        perror("Error al cargar semaforos");
        return error;
    }

    del1.semR = enc.semR1;
    del1.semW = enc.semW1;
    del2.semR = enc.semR2;
    del2.semW = enc.semW2;

    //Creación de monitores y asignación
    coc1.monEC = CrearMonitor();
    coc1.monCD = CrearMonitor();
    coc2.monEC = coc1.monEC;
    coc2.monCD = coc1.monCD;
    coc3.monEC = coc1.monEC;
    coc3.monCD = coc1.monCD;
    del1.monCD = coc1.monCD;
    del2.monCD = coc1.monCD;
    enc.monEC = coc1.monEC;

    //Creación de mutexes y asignación
    enc.mtxEnc = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    enc.mtxTel = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    coc1.mtxVarLast = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));

    error = mutexInit(enc.mtxEnc, enc.mtxTel, coc1.mtxVarLast);
    if (error != 0)
    {
        perror("Error al cargar los mutex");
        return error;
    }

    tel.mtxTel = enc.mtxTel;
    tel.mtxEnc = enc.mtxEnc;
    coc2.mtxVarLast = coc1.mtxVarLast;
    coc3.mtxVarLast = coc1.mtxVarLast;

    //Creación mem. compartida y mapeo
    int memoria1 = 0;
    int memoria2 = 0;
    error = crearMemoriaCompartida(&enc, &memoria1, &memoria2);
    if (error != 0)
    {
        perror("Error al crear la memoria compartida");
        return error;
    }
    del1.pedMemComp = enc.pedMemComp1;
    del2.pedMemComp = enc.pedMemComp2;

    //Setear tiempo de juego en segundos
    int valTimer = setTiempo();

    //Asignamos el valTimer a timer
    tel.timer = &valTimer;

    //Creación de hilos e inicialización
    threads = (pthread_t *)calloc(8, sizeof(pthread_t));
    pthread_create(&threads[0], NULL, telefono, &tel);
    pthread_create(&threads[1], NULL, cocinero, &coc1);
    pthread_create(&threads[2], NULL, cocinero, &coc2);
    pthread_create(&threads[3], NULL, cocinero, &coc3);
    pthread_create(&threads[4], NULL, delivery, &del1);
    pthread_create(&threads[5], NULL, delivery, &del2);
    pthread_create(&threads[6], NULL, printOutput, &out);
    pthread_create(&threads[7], NULL, takeInput, &optionSelected);

    //Implementación de encargado
    totalEarned = FuncionesDeEncargado(&enc, &tel, &optionSelected);

    pthread_join(threads[4], NULL);
    pthread_join(threads[5], NULL);

    printf("Cerrando local... \n\r");

    //Liberando memoria
    system("stty cooked");
    error = liberarMemoria(&enc, &memoria1, &memoria2);
    if (error != 0)
    {
        perror("Error al crear la memoria compartida");
        return error;
    }

    printf("Total ganado: %i\n", totalEarned);
    return error;
}

void *telefono(void *tmp)
{
    Telefono *tel = (Telefono *)tmp;
    int counter = 0;
    signal(SIGALRM, startTermination);
    alarm(*(tel->timer));
    while (1 == 1)
    {
        if (isFin == 1)
        {
            pthread_mutex_lock(tel->mtxTel);
            tel->pedActual = (Pedido *)calloc(1, sizeof(Pedido));
            tel->pedActual->distPed = (rand() % 5) + 1;
            tel->pedActual->estado = -2;
            tel->pedActual->nroPed = -1;
            tel->pedActual->tipoPed = (rand() % 5) + 1;
            pthread_mutex_unlock(tel->mtxTel);
            pthread_exit(NULL);
        }
        sleep(rand() % 6 + 2);           //Tiempo de espera a que el pedido suene
        pthread_mutex_lock(tel->mtxTel); //Se crea el pedido
        tel->pedActual = (Pedido *)calloc(1, sizeof(Pedido));
        tel->pedActual->distPed = (rand() % 5) + 1;
        tel->pedActual->estado = 0;
        tel->pedActual->nroPed = counter;
        tel->pedActual->tipoPed = (rand() % 5) + 1;
        counter++;
        pthread_mutex_unlock(tel->mtxTel);
        sleep(rand() % 11 + 5);          //Teléfono sonando
        if (tel->pedActual->estado == 0) //Si el pedido sigue teniendo el mismo estado, significa que no ha sido atendido por el encargado.
        {
            tel->pedActual->estado = -1; //Se le asigna -1, simbolizando el estado perdido.
        }
    }
}

void *cocinero(void *tmp)
{
    Cocinero *coc = (Cocinero *)tmp;
    coc->pedActual = calloc(1, sizeof(Pedido));
    while (1 == 1)
    {
        LeerDato(coc->monEC, coc->pedActual);

        if (coc->pedActual->estado == -2)
        {
            pthread_mutex_lock(coc->mtxVarLast);
            if (*(coc->last) == 1)
            {
                for (int i = 0; i < 3; i++)
                    GuardarDato(coc->monCD, coc->pedActual);
                pthread_mutex_unlock(coc->mtxVarLast);
                pthread_exit(NULL);
            }
            else
            {
                *(coc->last) -= 1;
                pthread_mutex_unlock(coc->mtxVarLast);
                pthread_exit(NULL);
            }
        }
        coc->pedActual->estado = 2;
        sleep(coc->pedActual->tipoPed);
        coc->pedActual->estado = 3;
        GuardarDato(coc->monCD, coc->pedActual);
        coc->pedActual->estado = 4;
    }
}

void *delivery(void *tmp)
{
    Delivery *del = (Delivery *)tmp;
    del->pedActual = calloc(1, sizeof(Pedido));
    while (1 == 1)
    {
        LeerDato(del->monCD, del->pedActual);
        if (del->pedActual->estado == -2)
        {
            pthread_exit(NULL);
        }
        del->pedActual->estado = 5;
        sleep(del->pedActual->distPed);
        del->pedActual->estado = 6;
        sem_wait(del->semW);
        del->pedMemComp->distPed = del->pedActual->distPed;
        del->pedMemComp->estado = del->pedActual->estado;
        del->pedMemComp->nroPed = del->pedActual->nroPed;
        del->pedMemComp->tipoPed = del->pedActual->tipoPed;
        del->pedActual->estado = 7;
        sem_post(del->semR);
    }
}

void *printOutput(void *tmp)
{
    Output *out = (Output *)tmp;
    int conteo = 1;
    while (!isFin)
    {
        system("clear");
        printf("\t\t\tTiempo elapsado: %i\n\r", conteo);
        printf("1) Atender\n\r2) Entregar pedido a cocinero\n\r3) Recolectar dinero delivery 1\n\r4) Recolectar dinero delivery 2\n\r");
        printf("Telefono(1): ");
        if (!(*(out->pedTel)))
            printf("No inicializado");
        else
        {
            if ((*(out->pedTel))->estado == 0)
                printf("Sonando");
            if ((*(out->pedTel))->estado == -1)
                printf("Llamada perdida");
            if ((*(out->pedTel))->estado == 1)
                printf("Atendido");
        }
        printf("\n\r");
        printf("Cocinero 1(2): ");
        if (!(*(out->pedCoc1)))
            printf("No inicializado");
        else
        {
            if ((*(out->pedCoc1))->estado == 2)
                printf("Preparando pedido");
            if ((*(out->pedCoc1))->estado == 3)
                printf("Entregando a Delivery");
            if ((*(out->pedCoc1))->estado == 4)
                printf("Esperando pedido");
        }
        printf("\n\r");
        printf("Cocinero 2(2): ");
        if (!(*(out->pedCoc2)))
            printf("No inicializado");
        else
        {
            if ((*(out->pedCoc2))->estado == 2)
                printf("Preparando pedido");
            if ((*(out->pedCoc2))->estado == 3)
                printf("Entregando a Delivery");
            if ((*(out->pedCoc2))->estado == 4)
                printf("Esperando pedido");
        }
        printf("\n\r");
        printf("Cocinero 3(2): ");
        if (!(*(out->pedCoc3)))
            printf("No inicializado");
        else
        {
            if ((*(out->pedCoc3))->estado == 2)
                printf("Preparando pedido");
            if ((*(out->pedCoc3))->estado == 3)
                printf("Entregando a Delivery");
            if ((*(out->pedCoc3))->estado == 4)
                printf("Esperando pedido");
        }
        printf("\n\r");
        printf("Delivery 1(3): ");
        if (!(*(out->pedDel1)))
            printf("No inicializado");
        else
        {
            if ((*(out->pedDel1))->estado == 5)
                printf("Entregado Pedido");
            if ((*(out->pedDel1))->estado == 6)
                printf("Entregando a Encargado");
            if ((*(out->pedDel1))->estado == 7)
                printf("En espera");
        }
        printf("\n\r");
        printf("Delivery 2(4): ");
        if (!(*(out->pedDel2)))
            printf("No inicializado");
        else
        {
            if ((*(out->pedDel2))->estado == 5)
                printf("Entregado Pedido");
            if ((*(out->pedDel2))->estado == 6)
                printf("Entregando a Encargado");
            if ((*(out->pedDel2))->estado == 7)
                printf("En espera");
        }
        printf("\n\r");
        conteo += 1;
        sleep(1);
    }
    pthread_exit(NULL);
}

void *takeInput(void *tmp)
{
    int *val = (int *)tmp;
    char a;
    // Seteamos la terminal en modo raw, para que se lea el input automáticamente al apretar un botón
    while (!isFin)
    {
        system("stty raw");
        a = getchar();
        *val = atoi(&a);
        if (a == '.')
        {
            system("stty cooked");
            raise(SIGALRM);
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}

void startTermination(int sig)
{
    isFin = 1;
}

int mutexInit(pthread_mutex_t *mtxEnc, pthread_mutex_t *mtxTel, pthread_mutex_t *mtxVarLast)
{
    int error = 0;
    error = pthread_mutex_init(mtxEnc, NULL);
    if (error)
    {
        perror("pthread_mutex_init()");
    }
    error += pthread_mutex_init(mtxTel, NULL);
    if (error)
    {
        perror("pthread_mutex_init()");
    }
    error += pthread_mutex_init(mtxVarLast, NULL);
    if (error)
    {
        perror("pthread_mutex_init()");
    }
    return error;
}

int crearSemaforos(Encargado *enc)
{
    int error = 0;
    enc->semR1 = sem_open("/semDel1R", O_CREAT, O_RDWR, 0);
    if (enc->semR1 == SEM_FAILED)
    {
        perror("sem_open()");
        error++;
    }
    enc->semW1 = sem_open("/semDel1W", O_CREAT, O_RDWR, 1);
    if (enc->semW1 == SEM_FAILED)
    {
        perror("sem_open()");
        error++;
    }
    enc->semR2 = sem_open("/semDel2R", O_CREAT, O_RDWR, 0);
    if (enc->semR2 == SEM_FAILED)
    {
        perror("sem_open()");
        error++;
    }
    enc->semW2 = sem_open("/semDel2W", O_CREAT, O_RDWR, 1);
    if (enc->semW2 == SEM_FAILED)
    {
        perror("sem_open()");
        error++;
    }

    return error;
}

int setTiempo()
{
    int valCorrecto = 0;
    int valTimer = 0;
    while (valCorrecto == 0)
    {
        printf("Por favor, ingrese la cantidad de segundos de juego (Mayor a 30): ");
        scanf("%d", &valTimer);
        printf("\n");
        if (valTimer > 30)
        {
            valCorrecto = 1;
        }
        else
        {
            printf("Por favor, ingrese un valor mayor a 30 \n");
        }
    }
    return valTimer;
}

int crearMemoriaCompartida(Encargado *enc, int *mem1, int *mem2)
{
    int error = 0;

    int memoria1 = shm_open("/memComp1", O_CREAT | O_RDWR, 0660);
    if (memoria1 < 0)
    {
        perror("shm_open()");
        error++;
    }
    if (!error)
    {
        error = ftruncate(memoria1, sizeof(Pedido));
        if (error)
            perror("ftruncate()");
    }

    int memoria2 = shm_open("/memComp2", O_CREAT | O_RDWR, 0660);
    if (memoria2 < 0)
    {
        perror("shm_open()");
        error++;
    }
    if (!error)
    {
        error = ftruncate(memoria2, sizeof(Pedido));
        if (error)
            perror("ftruncate()");
    }

    enc->pedMemComp1 = mmap(NULL, sizeof(Pedido), PROT_READ | PROT_WRITE, MAP_SHARED, memoria1, 0);
    if (enc->pedMemComp1 == MAP_FAILED)
    {
        perror("mmap()");
        error++;
    }

    enc->pedMemComp2 = mmap(NULL, sizeof(Pedido), PROT_READ | PROT_WRITE, MAP_SHARED, memoria2, 0);
    if (enc->pedMemComp2 == MAP_FAILED)
    {
        perror("mmap()");
        error++;
    }

    *mem1 = memoria1;
    *mem2 = memoria2;

    return error;
}

int liberarMemoria(Encargado *enc, int *mem1, int *mem2)
{
    int error = 0;
    error += sem_close(enc->semR1);
    if (error)
    {
        perror("sem_close()");
    }
    if (!error)
    {
        error = sem_unlink("/semDel1R");
        if (error)
            perror("sem_unlink()");
    }

    error += sem_close(enc->semW1);
    if (error)
    {
        perror("sem_close()");
    }
    if (!error)
    {
        error = sem_unlink("/semDel1W");
        if (error)
            perror("sem_unlink()");
    }

    error += sem_close(enc->semR2);
    if (error)
    {
        perror("sem_close()");
    }
    if (!error)
    {
        error = sem_unlink("/semDel2R");
        if (error)
            perror("sem_unlink()");
    }

    error += sem_close(enc->semW2);
    if (error)
    {
        perror("sem_close()");
    }
    if (!error)
    {
        error = sem_unlink("/semDel2W");
        if (error)
            perror("sem_close()");
    }
    error += munmap((void *)(enc->pedMemComp1), sizeof(Pedido));
    if (error)
    {
        perror("munmap()");
    }
    if (*mem1 > 0)
    {
        error += shm_unlink("/memComp1");
        if (error)
        {
            perror("unlink()");
        }
    }
    error += munmap((void *)(enc->pedMemComp2), sizeof(Pedido));
    if (error)
    {
        perror("munmap()");
    }
    if (*mem2 > 0)
    {
        error += shm_unlink("/memComp2");
        if (error)
        {
            perror("unlink()");
        }
    }
    return error;
}

int FuncionesDeEncargado(Encargado *enc, Telefono *tel, int *op)
{
    int totalEarned = 0;
    int fin = 1;

    while (fin)
    {
        if (tel->pedActual)
        {
            if (tel->pedActual->estado == -2)
            {
                enc->pedActual = tel->pedActual;
                for (int i = 0; i < 3; i++)
                {
                    GuardarDato(enc->monEC, enc->pedActual);
                }
                fin = 0;
            }
        }

        switch (*op)
        {
        case 1:
            pthread_mutex_lock(tel->mtxTel);
            enc->pedActual = tel->pedActual;
            enc->pedActual->estado = 1;
            pthread_mutex_unlock(tel->mtxTel);
            *op = 0;
            break;
        case 2:
            GuardarDato(enc->monEC, enc->pedActual);
            *op = 0;
            break;
        case 3:
            if (sem_trywait(enc->semR1))
            {
                totalEarned += enc->pedMemComp1->tipoPed * 10;
                sem_post(enc->semW1);

            }
            *op = 0;
            break;
        case 4:
            if (sem_trywait(enc->semR2))
            {
                totalEarned += enc->pedMemComp2->tipoPed * 10;
                sem_post(enc->semW2);
            }
            *op = 0;
            break;
        default:
            break;
        }
    }
    return totalEarned;
}
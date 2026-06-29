#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

void *banker(void *ptr);
void *watcher(void *ptr);  // CORREGIDO: ahora tiene el parámetro void*

int account = 0;
int last_printed = 0;  // Guarda el último múltiplo de 1000 que se imprimió
pthread_mutex_t transfer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t transfer_var = PTHREAD_COND_INITIALIZER;

void handle_pthread_error(char* message, int error) {
    fprintf(stderr, "PTHREAD ERROR: %s in %s\n", strerror(error), message);
    exit(EXIT_FAILURE);
}

int main() {
    int bCount = 16;
    pthread_t threads[bCount], watcherT;
    int tret[16];

    long i;
    for(i = 0; i < bCount; i++) {
        tret[i] = pthread_create(&threads[i], NULL, banker, (void*)i);
        if (tret[i]) {
            handle_pthread_error("banker thread creation", tret[i]);
        }
    }

    int wret = pthread_create(&watcherT, NULL, watcher, NULL);
    if (wret) {
        handle_pthread_error("watcher thread creation", wret);
    }

    for(i = 0; i < bCount; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_join(watcherT, NULL);

    exit(EXIT_SUCCESS);
}

void *banker(void* ptr) {
    long nr = (long) ptr;
    int ret = 0;

    while(1) {
        ret = pthread_mutex_lock(&transfer_mutex);
        if (ret) {
            handle_pthread_error("mutex lock", ret);
        }

        int value = 100;
        account += value;
        printf("banker nr %ld added %d, account: %d\n", nr, value, account);

        if (account % 1000 == 0) {
            ret = pthread_cond_signal(&transfer_var);
            if (ret) {
                handle_pthread_error("signal", ret);
            }
            printf("  Watcher signaled\n");
        }

        ret = pthread_mutex_unlock(&transfer_mutex);
        if (ret) {
            handle_pthread_error("mutex unlock", ret);
        }

        usleep(500 * 1000);
    }
}

void *watcher(void *ptr) {  // CORREGIDO: ahora tiene el parámetro void* ptr
    printf("Account watcher start\n");
    int ret = 0;
    
    while(1) {
        ret = pthread_mutex_lock(&transfer_mutex);
        if (ret) {
            handle_pthread_error("mutex lock", ret);
        }

        // Esperar hasta que account sea múltiplo de 1000 Y sea un nuevo múltiplo no impreso
        while (account % 1000 != 0 || account == last_printed) {
            ret = pthread_cond_wait(&transfer_var, &transfer_mutex);
            if (ret) {
                handle_pthread_error("cond_wait", ret);
            }
        }

        // Solo imprimir si es un nuevo múltiplo de 1000
        if (account % 1000 == 0 && account != last_printed) {
            printf("\t*** Account watcher: account = %d (múltiplo de 1000) ***\n", account);
            last_printed = account;  // Recordar que ya imprimimos este valor
        }

        ret = pthread_mutex_unlock(&transfer_mutex);
        if (ret) {
            handle_pthread_error("mutex unlock", ret);
        }
        
        // Pequeña pausa para no saturar la CPU
        usleep(100 * 1000);
    }
}

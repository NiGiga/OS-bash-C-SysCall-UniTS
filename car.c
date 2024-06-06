/*
sviluppare un programma per la gestione di un autonoleggio con più operatori concorrenti.

Funzionalita' da sviluppare:

Gestione del parco macchine:

Il programma deve leggere un file catalog.txt che contiene gli identificativi delle vetture.
Il programma deve permettere di visualizzare lo stato delle vetture (libere o occupate), noleggiare una vettura (lock) e rilasciare una vettura (release).
Comandi del programma:

view: visualizza lo stato delle vetture.
lock <vettura>: noleggia una vettura se non è già noleggiata.
release <vettura>: rilascia una vettura noleggiata.
quit: termina il programma.
Gestione concorrente:

Il programma deve supportare più operatori simultanei.
Si consiglia l'uso di semafori nominati per gestire lo stato delle vetture.
Persistenza dello stato:

Lo stato del programma deve persistere tra i riavvii.


*/
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_CARS 100
#define SEM_PREFIX "/car_semaphore_"
#define STATE_FILE "car_state.txt" //nome file per salvataggio stato dopo quit

// Definizione della struttura Car
typedef struct {
    char id[20];    // Identificativo della vettura
    char status[10]; // Stato della vettura (free o busy)
} Car;

// Array di vetture e contatore delle vetture
Car cars[MAX_CARS];
int car_count = 0;

// Funzione per caricare il catalogo delle vetture dal file catalog.txt
void load_catalog() {
    FILE *file = fopen("catalog.txt", "r"); // Apro il file catalog.txt in modalità lettura
    if (file == NULL) {
        perror("Error opening catalog file"); // Stampo un messaggio di errore se il file non può essere aperto
        exit(EXIT_FAILURE); // Esco dal programma con codice di errore
    }

    // Leggo ogni identificativo di vettura e lo memorizzo nell'array cars
    while (fscanf(file, "%s", cars[car_count].id) != EOF) {
        car_count++; // Incremento il contatore delle vetture
    }

    fclose(file); // Chiudo il file

    // Provo a caricare lo stato dal file di stato
    FILE *state_file = fopen(STATE_FILE, "r");
    if (state_file != NULL) {
        for (int i = 0; i < car_count; i++) {
            fscanf(state_file, "%s %s", cars[i].id, cars[i].status); // Carico l'identificativo e lo stato
        }
        fclose(state_file);
    } else {
        // Se il file di stato non esiste, imposto tutte le vetture a "free"
        for (int i = 0; i < car_count; i++) {
            strcpy(cars[i].status, "free");
        }
    }
}

// Funzione per inizializzare i semafori nominati per ciascuna vettura
void init_semaphores() {
    for (int i = 0; i < car_count; i++) { // Ciclo attraverso tutte le vetture nel catalogo
        char sem_name[50]; // Array per memorizzare il nome del semaforo
        snprintf(sem_name, 50, "%s%s", SEM_PREFIX, cars[i].id); // Creo il nome del semaforo usando il prefisso e l'identificativo della vettura
        sem_t *sem = sem_open(sem_name, O_CREAT, 0644, 1); // Creo il semaforo con valore iniziale 1
        if (sem == SEM_FAILED) { // Controllo se la creazione del semaforo è fallita
            perror("Error creating semaphore"); // Stampo un messaggio di errore
            exit(EXIT_FAILURE); // Esco dal programma con codice di errore
        }
        sem_close(sem); // Chiudo il semaforo
    }
}

// Funzione per visualizzare lo stato delle vetture
void view() {
    for (int i = 0; i < car_count; i++) { // Ciclo attraverso tutte le vetture nel catalogo
        printf("Car: %s, status: %s\n", cars[i].id, cars[i].status); // Stampo l'identificativo e lo stato di ciascuna vettura
    }
}

// Funzione per trovare l'indice di una vettura nell'array cars
int find_car_index(char *car_id) {
    for (int i = 0; i < car_count; i++) { // Ciclo attraverso tutte le vetture nel catalogo
        if (strcmp(cars[i].id, car_id) == 0) { // Confronto l'identificativo della vettura con l'identificativo cercato
            return i; // Ritorno l'indice della vettura se trovata
        }
    }
    return -1; // Ritorno -1 se la vettura non è trovata
}

// Funzione per noleggiare una vettura
void lock(char *car_id) {
    int index = find_car_index(car_id); // Trovo l'indice della vettura usando il suo identificativo
    if (index == -1) { // Controllo se la vettura non esiste nel catalogo
        printf("Cannot find car %s\n", car_id); // Stampo un messaggio di errore
        return; // Esco dalla funzione
    }

    char sem_name[50]; // Array per memorizzare il nome del semaforo
    snprintf(sem_name, 50, "%s%s", SEM_PREFIX, car_id); // Creo il nome del semaforo usando il prefisso e l'identificativo della vettura
    sem_t *sem = sem_open(sem_name, 0); // Apro il semaforo esistente
    sem_wait(sem); // Attendo che il semaforo diventi disponibile

    if (strcmp(cars[index].status, "free") == 0) { // Controllo se la vettura è libera
        strcpy(cars[index].status, "busy"); // Cambio lo stato della vettura a "busy"
        printf("Car: %s is now locked\n", car_id); // Stampo un messaggio di successo
    } else {
        printf("Error. Car %s already locked\n", car_id); // Stampo un messaggio di errore se la vettura è già noleggiata
    }

    sem_post(sem); // Rilascio il semaforo
    sem_close(sem); // Chiudo il semaforo
}

// Funzione per rilasciare una vettura
void release(char *car_id) {
    int index = find_car_index(car_id); // Trovo l'indice della vettura usando il suo identificativo
    if (index == -1) { // Controllo se la vettura non esiste nel catalogo
        printf("Cannot find car %s\n", car_id); // Stampo un messaggio di errore
        return; // Esco dalla funzione
    }

    char sem_name[50]; // Array per memorizzare il nome del semaforo
    snprintf(sem_name, 50, "%s%s", SEM_PREFIX, car_id); // Creo il nome del semaforo usando il prefisso e l'identificativo della vettura
    sem_t *sem = sem_open(sem_name, 0); // Apro il semaforo esistente
    sem_wait(sem); // Attendo che il semaforo diventi disponibile

    if (strcmp(cars[index].status, "busy") == 0) { // Controllo se la vettura è noleggiata
        strcpy(cars[index].status, "free"); // Cambio lo stato della vettura a "free"
        printf("Car: %s is now free\n", car_id); // Stampo un messaggio di successo
    } else {
        printf("Error. Car %s already free\n", car_id); // Stampo un messaggio di errore se la vettura è già libera
    }

    sem_post(sem); // Rilascio il semaforo
    sem_close(sem); // Chiudo il semaforo
}

// Funzione per salvare lo stato delle vetture su un file
void save_state() {
    FILE *state_file = fopen(STATE_FILE, "w"); // Apro il file di stato in modalità scrittura
    if (state_file == NULL) {
        perror("Error opening state file"); // Stampo un messaggio di errore se il file non può essere aperto
        exit(EXIT_FAILURE); // Esco dal programma con codice di errore
    }

    // Scrivo l'identificativo e lo stato di ciascuna vettura nel file
    for (int i = 0; i < car_count; i++) {
        fprintf(state_file, "%s %s\n", cars[i].id, cars[i].status);
    }

    fclose(state_file); // Chiudo il file
}

// Funzione per terminare il programma e rimuovere i semafori
void quit() {
    save_state(); // Salvo lo stato delle vetture
    for (int i = 0; i < car_count; i++) { // Ciclo attraverso tutte le vetture nel catalogo
        char sem_name[50]; // Array per memorizzare il nome del semaforo
        snprintf(sem_name, 50, "%s%s", SEM_PREFIX, cars[i].id); // Creo il nome del semaforo usando il prefisso e l'identificativo della vettura
        sem_unlink(sem_name); // Rimuovo il semaforo
    }
    exit(EXIT_SUCCESS); // Esco dal programma con codice di successo
}

int main() {
    load_catalog();  // Carico il catalogo delle vetture
    init_semaphores();  // Inizializzo i semafori per le vetture

    char command[100]; // Array per memorizzare il comando inserito dall'utente
    char car_id[20]; // Array per memorizzare l'identificativo della vettura inserito dall'utente

    while (1) { // Ciclo infinito per attendere comandi dall'utente
        printf("Command: "); // Stampo il prompt del comando
        scanf("%s", command); // Leggo il comando dall'utente

        // Verifico quale comando è stato inserito e chiamo la funzione corrispondente
        if (strcmp(command, "view") == 0) { // Se il comando è "view"
            view(); // Chiamo la funzione view
        } else if (strcmp(command, "lock") == 0) { // Se il comando è "lock"
            scanf("%s", car_id); // Leggo l'identificativo della vettura dall'utente
            lock(car_id); // Chiamo la funzione lock
        } else if (strcmp(command, "release") == 0) { // Se il comando è "release"
            scanf("%s", car_id); // Leggo l'identificativo della vettura dall'utente
            release(car_id); // Chiamo la funzione release
        } else if (strcmp(command, "quit") == 0) { // Se il comando è "quit"
            quit(); // Chiamo la funzione quit
        } else { // Se il comando è sconosciuto
            printf("Unknown Command\n"); // Stampo un messaggio di errore
        }
    }

    return 0; // Ritorno 0, anche se questo punto non sarà mai raggiunto
}

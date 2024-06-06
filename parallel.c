/*
Si realizzi in C una versione semplificata del comando GNU parallel .
Il programma deve accettare tre argomenti:
    1. Il nome di un file dove il programma legge, uno per riga , i parametri del comando che deve eseguire
    2. Un numero che rappresenta quanti processi in parallelo deve eseguire
    3. Una stringa che rappresenta il comando da eseguire. La stringa deve contenere il carattere % che verrà sostituito dal programma con i parametri letti dal file passato come primo argomento.
Esempio:
Se viene eseguito ./parallel args.txt 2 "ls % - lh " e il file args.txt contiene:
    /etc/resolv.conf
    /etc/fstab
    /etc/hostname
    /etc/hosts
Il programma deve leggere le 4 righe dal file args.txt e sostituirle al carattere % nella stringa passata come terzo argoment o per creare 4 comandi che esso deve eseguire usando 2 
processi in parallelo. I comandi da eseguire saranno pertanto:
    ls /etc/resolv.conf -lh
    ls /etc/fstab -lh
    ls /etc/hostname -lh
    ls /etc/hosts -lh
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

// Definisco i limiti massimi per il numero di processi e la lunghezza di un comando
#define MAXPROCESSES 256
#define MAXCMD 256

// Funzione per chiudere tutte le estremità di scrittura delle pipe
void close_pipes(int pipes[MAXPROCESSES][2], int n) {
    for (int i = 0; i < n; i++) {
        close(pipes[i][1]);  // Chiudo l'estremità di scrittura della pipe
    }
}

// Funzione per sostituire il carattere '%' nel comando con il parametro
void replace_percent(char *command, char *param, char *result) {
    char *pos = strstr(command, "%");  // Trovo la posizione del carattere '%'
    if (pos == NULL) {  // Se non trovo '%', copio il comando così com'è
        strcpy(result, command);
        return;
    }
    // Copio la parte del comando prima di '%'
    strncpy(result, command, pos - command);
    result[pos - command] = '\0';  // Aggiungo il terminatore di stringa
    strcat(result, param);  // Aggiungo il parametro
    strcat(result, pos + 1);  // Aggiungo la parte del comando dopo '%'
}

int main(int argc, char *argv[]) {
    // Controllo che il numero di argomenti sia corretto
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file> <num_processes> <command>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Estraggo il nome del file e il numero di processi
    char *filename = argv[1];
    char *endptr;
    errno = 0;  // Resetto errno prima della chiamata a strtol
    long num_processes = strtol(argv[2], &endptr, 10);  // Converto il numero di processi

    // Controllo se ci sono stati errori nella conversione
    if (errno != 0 || *endptr != '\0' || num_processes <= 0 || num_processes > MAXPROCESSES) {
        fprintf(stderr, "Invalid number of processes. Must be between 1 and %d\n", MAXPROCESSES);
        exit(EXIT_FAILURE);
    }

    char *command = argv[3];  // Estraggo il comando da eseguire

    // Apro il file contenente i parametri dei comandi
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");  // Stampo un messaggio di errore se non riesco ad aprire il file
        exit(EXIT_FAILURE);
    }

    // Creo le pipe per la comunicazione tra il processo padre e i processi figli
    int pipes[MAXPROCESSES][2];
    for (int i = 0; i < num_processes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Failed to create pipe");  // Stampo un messaggio di errore se non riesco a creare una pipe
            exit(EXIT_FAILURE);
        }
    }

    // Creo i processi figli usando fork()
    pid_t pids[MAXPROCESSES];
    for (int i = 0; i < num_processes; i++) {
        pids[i] = fork();  // Creo un nuovo processo figlio
        if (pids[i] == -1) {
            perror("Failed to fork");  // Stampo un messaggio di errore se fork() fallisce
            exit(EXIT_FAILURE);
        } else if (pids[i] == 0) {  // Questo blocco di codice viene eseguito dai processi figli
            close(pipes[i][1]);  // Chiudo l'estremità di scrittura della pipe nel processo figlio

            char cmd[MAXCMD];
            while (1) {
                int bytesRead = read(pipes[i][0], cmd, MAXCMD);
                if (bytesRead == 0) break;  // Se non ci sono più dati da leggere, esco dal ciclo
                if (bytesRead < 0) {  // Se c'è un errore nella lettura, stampo un messaggio di errore e esco
                    perror("Failed to read from pipe");
                    exit(EXIT_FAILURE);
                }
                cmd[bytesRead] = '\0';  // Assicuro che la stringa letta sia null-terminata
                printf("Executing command: %s\n", cmd);  // Messaggio di debug
                system(cmd);  // Eseguo il comando letto dalla pipe
            }

            close(pipes[i][0]);  // Chiudo l'estremità di lettura della pipe
            exit(EXIT_SUCCESS);  // Termino il processo figlio dopo aver eseguito tutti i comandi
        } else {  // Questo blocco di codice viene eseguito dal processo padre
            close(pipes[i][0]);  // Chiudo l'estremità di lettura della pipe nel processo padre
        }
    }

    // Leggo i parametri dal file e li invio ai processi figli attraverso le pipe
    char param[MAXCMD];
    int command_count = 0;
    while (fgets(param, MAXCMD, file) != NULL) {
        param[strcspn(param, "\n")] = '\0';  // Rimuovo il carattere di newline dai parametri letti

        if (param[0] == '\0') {
            continue;  // Salto le linee vuote
        }

        char full_command[MAXCMD];
        replace_percent(command, param, full_command);  // Sostituisco '%' con il parametro

        if (strlen(full_command) >= MAXCMD) {
            fprintf(stderr, "Command is too long\n");  // Stampo un messaggio di errore se il comando è troppo lungo
            exit(EXIT_FAILURE);
        }

        int target_process = command_count % num_processes;  // Determino quale figlio deve eseguire il comando corrente
        printf("Sending command to process %d: %s\n", target_process, full_command);  // Messaggio di debug
        if (write(pipes[target_process][1], full_command, strlen(full_command) + 1) == -1) {
            perror("Failed to write to pipe");  // Stampo un messaggio di errore se non riesco a scrivere nella pipe
            exit(EXIT_FAILURE);
        }

        command_count++;  // Incremento il conteggio dei comandi
    }

    fclose(file);  // Chiudo il file

    // Chiudo tutte le estremità di scrittura delle pipe per segnalare ai figli che non ci sono più comandi da eseguire
    close_pipes(pipes, num_processes);

    // Attendo la terminazione di tutti i processi figli
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);  // Il processo padre aspetta che ogni figlio termini
    }

    return 0;  // Termino il programma con successo
}

#include <stdio.h>      // Per funzioni di input/output come printf
#include <sys/types.h>  // Definisce tipi come uid_t, gid_t, etc.
#include <sys/stat.h>   // Definisce la struttura stat per ottenere informazioni sui file
#include <unistd.h>     // Contiene funzioni come lstat per ottenere informazioni sui file
#include <dirent.h>     // Per gestire directory e funzioni come opendir, readdir, etc.
#include <pwd.h>        // Per ottenere informazioni sugli utenti tramite getpwuid
#include <grp.h>        // Per ottenere informazioni sui gruppi tramite getgrgid
#include <string.h>     // Per funzioni string come strcmp e snprintf

/**
 * Funzione per stampare i dettagli di un file/directory dato il suo percorso e la struttura stat
 * @param path: Percorso del file o della directory
 * @param statbuf: Puntatore alla struttura stat contenente le informazioni del file
 */
void print_info(const char *path, struct stat *statbuf) {
    // Ottengo le informazioni dell'utente proprietario usando l'ID utente
    struct passwd *pw = getpwuid(statbuf->st_uid);
    // Ottengo le informazioni del gruppo proprietario usando l'ID gruppo
    struct group *gr = getgrgid(statbuf->st_gid);

    // Determino il tipo di file (file, directory, symbolic link, FIFO, altro)
    const char *type;
    if (S_ISREG(statbuf->st_mode)) type = "file";
    else if (S_ISDIR(statbuf->st_mode)) type = "directory";
    else if (S_ISLNK(statbuf->st_mode)) type = "symbolic link";
    else if (S_ISFIFO(statbuf->st_mode)) type = "FIFO";
    else type = "other";

    // Stampo i dettagli secondo il formato richiesto
    printf("Node: %s\n", path);
    printf("    Inode: %ld\n", (long)statbuf->st_ino);
    printf("    Type: %s\n", type);
    printf("    Size: %ld\n", (long)statbuf->st_size);
    printf("    Owner: %d %s\n", statbuf->st_uid, pw ? pw->pw_name : "unknown");
    printf("    Group: %d %s\n", statbuf->st_gid, gr ? gr->gr_name : "unknown");
}

/**
 * Funzione per attraversare ricorsivamente una directory e stampare i dettagli dei file contenuti
 * @param dirpath: Percorso della directory da attraversare
 */
void traverse_directory(const char *dirpath) {
    struct dirent *entry;         // Puntatore a struct dirent per leggere le directory
    struct stat statbuf;          // Buffer per contenere le informazioni del file
    DIR *dir = opendir(dirpath);  // Apro la directory specificata
    if (!dir) {
        // Gestisco gli errori nell'apertura della directory
        perror("opendir");
        return;
    }

    // Ciclo attraverso tutte le voci della directory
    while ((entry = readdir(dir)) != NULL) {
        // Ignoro le directory speciali "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Creo il percorso completo del file/directory
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

        // Ottengo le informazioni del file o directory con lstat
        if (lstat(fullpath, &statbuf) == -1) {
            perror("lstat");
            continue;
        }

        // Stampo le informazioni del file/directory
        print_info(fullpath, &statbuf);

        // Se è una directory, la attraversiamo ricorsivamente
        if (S_ISDIR(statbuf.st_mode)) {
            traverse_directory(fullpath);
        }
    }

    // Chiudo la directory dopo averla attraversata
    closedir(dir);
}

/**
 * Funzione principale del programma
 * @param argc: Numero di argomenti passati tramite riga di comando
 * @param argv: Array degli argomenti passati tramite riga di comando
 * @return: Codice di uscita
 */
int main(int argc, char *argv[]) {
    // Verifico se è stato passato l'argomento corretto (percorso della directory)
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    // Ottengo le informazioni del percorso specificato
    struct stat statbuf;
    if (lstat(argv[1], &statbuf) == -1) {
        perror("lstat");
        return 1;
    }

    // Stampo le informazioni della directory/file iniziale
    print_info(argv[1], &statbuf);

    // Se è una directory, la attraversiamo ricorsivamente
    if (S_ISDIR(statbuf.st_mode)) {
        traverse_directory(argv[1]);
    }

    return 0;
}

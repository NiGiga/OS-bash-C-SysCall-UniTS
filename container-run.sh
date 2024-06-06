# Si realizzi in Bash una semplice Container Engine che funziona senza richiedere privilegi di amministratore.
# Essa deve permettere di eseguire un programma in un ambiente isolato.
# 
# Requisiti:
# - Deve essere possibile eseguire un programma specificando un file di configurazione e il comando da eseguire.
# - Il container deve utilizzare un ambiente temporaneo creato in una directory di lavoro temporanea.
# - Il file di configurazione deve contenere l'elenco dei binari necessari e delle directory da copiare o montare nell'ambiente isolato.
# - Deve utilizzare fakechroot e chroot per l'isolamento senza richiedere privilegi di amministratore.
# - Deve eseguire il programma specificato nel container e fornire l'output all'utente.
# - Alla fine dell'esecuzione, deve pulire l'ambiente temporaneo creato.

# Esempio di utilizzo:
# ./container-run.sh conf-file.txt /bin/ls -lh /lib



#!/bin/bash

# Controllo che siano stati passati almeno due argomenti
if [ "$#" -lt 2 ]; then
    echo "Uso: $0 conf-file command [args...]"
    exit 1
fi

# Variabili per il file di configurazione e il comando
CONF_FILE=$1
shift # rimuovo il primo argomento
COMMAND=("$@") # array di argomenti \{shift}


# Creo directory temporanea
WORKDIR="/tmp/container-$(date +%s)-$$" # Uso data e PID script
mkdir -p "$WORKDIR"
STATUS=$? # Variabile di controllo


# Controllo che la directory temporanea sia stata creata
if [ $STATUS -ne 0 ] || [ ! -d "$WORKDIR" ]; then
    echo "Errore: impossibile creare la directory temporanea"
    exit 1
fi

# Array per tenere traccia delle directory montate
MOUNTED_DIRS=()

# Funzione per copiare le librerie necessarie
# Prendo binario e directory di destinazione
copy_libs() {
    local bin="$1"
    local dest_dir="$2"
    mkdir -p "$dest_dir"

    # Uso ldd per trovare lib necessarie al binario e le copio
    for lib in $(ldd "$bin" | awk '{print $3}' | grep -v '('); do
        local lib_dir=$(dirname "$lib")
        mkdir -p "$dest_dir/$lib_dir"
        cp "$lib" "$dest_dir/$lib_dir"
    done
}

# Funzione per pulire la directory temporanea al termine
cleanup() {
    echo "Pulizia della directory temporanea"
    # Ciclo per pulire le directory
    for dir in "${MOUNTED_DIRS[@]}"; do
        if mountpoint -q "$dir"; then
            echo "Smontando $dir"
            # Uso fusermount -u per smontare le directory montate
            # prima di rimuoverle, per evitare problemi di permessi
            fusermount -u "$dir"
        fi
    done
    rm -rf "$WORKDIR"
}
trap cleanup EXIT # Chiamo cleanup anche se fallisco

# Imposto il separatore di campo interno (IFS) al carattere newline
IFS=$'\n'

# Leggo il file di configurazione e creo i bind mounts o copio i file e le librerie necessarie
while read -r line; do
    ORIGIN=$(echo "$line" | awk '{print $1}') # estraggo percorso origine con awk
    DST=$(echo "$line" | awk '{print $2}') # estraggo percorso destinazione con awk
    DST_DIR="$WORKDIR/$(dirname "$DST")" # creo e poi sostituisco percorso WORKDIR


    # Creo la directory di destinazione se non esiste
    mkdir -p "$DST_DIR"




     # Se ORIGIN e' directory uso bindfs per montarla
    if [ -d "$ORIGIN" ]; then
      # Gestisco "fuse: mountpoint is not empty"
      if mountpoint -q "$WORKDIR/$DST"; then
        echo "Il mount point $WORKDIR/$DST non è vuoto, continuando con --nonempty"
      else
        mkdir -p "$WORKDIR/$DST" # Per assicurarmi che esista
      fi

      bindfs --no-allow-other --nonempty "$ORIGIN" "$WORKDIR/$DST"
      MOUNTED_DIRS+=("$WORKDIR/$DST") # Aggiunte per pulizia succesiva

    # Se file copio
    elif [ -f "$ORIGIN" ]; then

        cp "$ORIGIN" "$WORKDIR/$DST"
        copy_libs "$ORIGIN" "$WORKDIR"

    # Altrimenti
    else
        echo "Errore: origine $ORIGIN non trovata"
        exit 1
    fi
done < "$CONF_FILE" # Prendo input per while da CONF_FILE


# Eseguo il comando specificato nel container con fakechroot
echo "Eseguendo fakechroot chroot $WORKDIR ${COMMAND[@]}"
fakechroot chroot "$WORKDIR" "${COMMAND[@]}"

# Evito la chiamata ridondante a cleanup
trap - EXIT
cleanup







































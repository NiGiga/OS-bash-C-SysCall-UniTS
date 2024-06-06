# Lab1 di Sistemi Operativi.
#L'obbiettivo e' di creare una semplice utility per gestire una rubrica in formato .csv. Si richiede in oltre l'implementazione di specifiche funzionalita':
# Visualizzazione: address-book.sh view -> stampa le voci della rubrica allineando gli elementi per colonne e ordinando le linee per mail (ordine alfabetico)
# le colonne dovranno essere : name surname phone mail city address
#
# Ricerca: address-book.sh search <string> -> ricerca nel documento per <string> e stampa le line contenenti tale voce.
# Esempio output:
#      Name: luca
#      Surname: rossi
#      Phone: 334515642
#      Mail:
#      luca@libero.it
#      City: trieste
#      Address: via giulia 23
#      Not found (nel caso non vi siano voci)
#
# Inserimento: address-book.sh insert -> inserimento di nuovi dati in rubrica 
# Esempio output:
#      Name: luca
#      Surname: rossi
#      Phone: 334515642
#      Mail:
#      luca@libero.it
#      City: trieste
#      Address: via giulia 23
#      Added
#
# Cancellazione: address-book.sh delete <mail> -> cancella un elemento usando come parametro la mail. Otput in caso di sucesso Delated in caso di fallimento: Not found


#!/bin/bash

DATABASE="address-book-database.csv"

# Funzione per visualizzare tutte le voci nella rubrica
view() {
    (head -n 1 "$DATABASE" && tail -n +2 "$DATABASE" | sort -t, -k4) | column -s, -t
}

# Funzione per cercare una stringa specifica all'interno della rubrica
search() {
    local keyword="$1"
    local found=false

    IFS=$'\n'
    for line in $(grep -i "$keyword" "$DATABASE"); do
        if [[ $line != $(head -n 1 "$DATABASE") ]]; then
            IFS=',' read -r name surname phone mail city address <<<"$line"
            echo -e "Name: $name\nSurname: $surname\nPhone: $phone\nMail: $mail\nCity: $city\nAddress: $address\n"
            found=true
        fi
    done

    if [[ $found == false ]]; then
        echo "Not found"
    fi
}

# Funzione per inserire una nuova voce nella rubrica
insert() {
    echo "Name: "
    read name
    echo "Surname: "
    read surname
    echo "Phone: "
    read phone
    echo "Mail: "
    read mail
    echo "City: "
    read city
    echo "Address: "
    read address

    if grep -q "^.*,.*,.*,${mail},.*,.*$" "$DATABASE"; then
        echo "Error: Mail already exists"
    else
        echo "$name,$surname,$phone,$mail,$city,$address" >>"$DATABASE"
        echo "Added"
    fi
}

# Funzione per cancellare una voce specifica dalla rubrica
delete() {
    local mail="$1"
    if grep -q "^.*,.*,.*,${mail},.*,.*$" "$DATABASE"; then
        grep -v "^.*,.*,.*,${mail},.*,.*$" "$DATABASE" > tmpfile
        (head -n 1 "$DATABASE" && cat tmpfile) > "$DATABASE"
        rm -f tmpfile
        echo "Deleted"
    else
        echo "Cannot find any record"
    fi
}

# Logica di controllo dei vari comandi
case "$1" in
    view) view ;;  # Se il comando è 'view', esegue la funzione 'view'
    search) search "$2" ;;  # Se il comando è 'search', esegue la funzione 'search' con il secondo argomento come parametro
    insert) insert ;;  # Se il comando è 'insert', esegue la funzione 'insert'
    delete) delete "$2" ;;  # Se il comando è 'delete', esegue la funzione 'delete' con il secondo argomento come parametro
    *)  # Default: se nessun comando corrisponde, stampa un messaggio di utilizzo
        echo "Usage: $0 {view|search <string>|insert|delete <mail>}"
        ;;
esac

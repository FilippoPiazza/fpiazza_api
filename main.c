// Filippo Piazza
// 2024
#define MAX_WORD_LENGTH 128
#define MAX_LINE_LENGTH 16384
#define _VERBOSE 0

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

const char *accettato = "accettato\n";
const char *aggiunto = "aggiunta\n";
const char *rifiutato = "rifiutato\n";
const char *ordinisospeso = "ordini in sospeso\n";
const char *rimosso = "rimossa\n";
const char *ignorato = "ignorato\n";
const char *non_presente = "non presente\n";


typedef struct magazzino
{
    struct magazzino *prev;
    char ingr_name[MAX_WORD_LENGTH];
    int tot_av;
    struct ingrediente *ingredienti;
    struct magazzino *next;
}magazzino;

typedef struct ingrediente
{
    int qta;
    int expiry;
    struct ingrediente *next;
}ingrediente ;

typedef struct  ingrediente_ricetta
{
    magazzino *ingr;
    int qta;
    struct ingrediente_ricetta *next;
} ingrediente_ricetta;

typedef struct ricetta
{
    struct ricetta *prev;
    char name[MAX_WORD_LENGTH];
    struct ingrediente_ricetta* ingredienti; // puntatore agli ingredienti
    int n_ord; // contatore per numero di ordini attivi sulla ricetta, 0 di default. Serve per l'eliminazione, va incrementato ad ogni ordine e decrementato a ogni consegna
    int total_qta;
    struct ricetta *next;
} ricetta;


typedef struct ordini
{
    int qta;
    struct ricetta* ricetta_ord;
    struct ordini *next;
    int time_placed;
} ordini;

typedef struct ordini_completi
{
    int qta;
    struct ricetta* ricetta_ord;
    int dim_tot; //dimensione totale ordine, utile per il carico del furgone
    int time_placed;
    struct ordini_completi *next;
}ordini_completi ;

typedef struct ordini_in_carico
{
    char name[MAX_WORD_LENGTH];
    int qta;
    int dim_tot;
    int time_placed;
    struct ordini_in_carico *next;
}ordini_in_carico;

void aggiungi_ricetta(const char* nome_ricetta, char** token_ingredienti);
void aggiungi_ordine(const char *nome_ricetta, const int quantita, const int t);
void prepara_ordini(const int current_time);
void rifornisci(char *buffer, const int t);
void verifica_scadenze(const int t);
void rimuovi_ricetta(const char* nome_ricetta);
void carica_furgone(const int max_cargo, int tempo);
ricetta* ricerca_pseudo_binaria(const char* nome_ricetta);
magazzino* ricerca_pseudo_binaria_ingr(const char* nome_ingr); //this and the above should be merged somehow
magazzino* punt_ingrediente(const char* nome_ingr);
int read_line_unlocked(char *buffer, int max_size);
void trim_newline(char *str);



int ricette_totali = 0;
int ingredienti_totali = 0;
ricetta* head_ricetta = NULL;
ordini *head_ordine = NULL;
ordini *tail_ordine = NULL;
magazzino *head_magazzino = NULL;
ordini_completi *head_ordine_completi = NULL;
ordini_in_carico *head_ordine_in_carico = NULL;


int main(void)
{

    char* buffer = (char*)alloca(MAX_LINE_LENGTH * sizeof(char));
    if (buffer == NULL) {
        perror("Failed to allocate buffer");
        return EXIT_FAILURE;
    }

    //input iniziale di configurazione del furgone
    if(read_line_unlocked(buffer, MAX_LINE_LENGTH) == 0){return 69420;}
    char *ptr = buffer;
    const int tempocorriere = strtol(ptr, &ptr, 10);
    const int max_cargo = strtol(ptr, &ptr, 10);
    //printf("%d%d\n", max_cargo, tempocorriere);
    memset(buffer, 0, MAX_LINE_LENGTH); // pulisce il buffer

    //la variabile cd_corriere funziona da countdown
    int cd_corriere = tempocorriere;


    //la variabile t conta il tempo
    int t = 0;

    while(1){
        verifica_scadenze(t);


        //controllo se arriva il corriere
        if (cd_corriere == 0) {
            carica_furgone(max_cargo, t);
            cd_corriere = tempocorriere;
            cd_corriere -=1;
        }
        else {cd_corriere -= 1;}
        /* comandi:
            aggiungi_ricetta
            ordine
            rimuovi_ricetta
            rifornimento
        */

        if(read_line_unlocked(buffer, MAX_LINE_LENGTH) == 0){break;}


        trim_newline(buffer);

        // se aggiungi_ricetta
        if(buffer[2] == 'g'){
            char *token = strtok(buffer + 17, " \t\n"); //verifica offset
            // *token è il nome della ricetta
            //if (token == NULL) return; \\ nome ricetta assente

            char* tokens[MAX_LINE_LENGTH]; // contiene il resto del comando
            int idx = 0;

            char *nome_ricetta = token;
            token = strtok(NULL, " \t\n");
            while ((token != NULL) && idx < (MAX_LINE_LENGTH / 2)) {
                tokens[idx] = token;
                idx += 1;
                token = strtok(NULL, " \t\n");
            }

            tokens[idx] = NULL; // aggiungo terminatore nullo alla lista dei token

            aggiungi_ricetta(nome_ricetta, tokens);
            }

        // se rimuovi_ricetta
        else if(buffer[2] == 'm'){
            char *token = strtok(buffer + 16, " "); //verifica offset
            rimuovi_ricetta(token);
        }

        // se ordine
        else if(buffer[2] == 'd'){
            char *nome_ricetta = strtok(buffer + 7, " "); //verifica offset
            char *qta_str = strtok(NULL, " \t\n");
            if (nome_ricetta == NULL || qta_str == NULL) {
                printf("Input non valido\n");
                continue;
            }
            int quantita = atoi(qta_str);
            aggiungi_ordine(nome_ricetta, quantita, t);
        }

        // se rifornimento
        else if(buffer[2] == 'f'){
            rifornisci(buffer, t);
            //prepara_ordini(t); // TODO ha senso preparare solo questi?
            //rifornimento_flag = 1;

        }

        prepara_ordini(-1); // verifica

        t += 1;
        memset(buffer, 0, MAX_LINE_LENGTH); // pulisce il buffer


    }

}

void aggiungi_ricetta(const char* nome_ricetta, char** token_ingredienti) {

    ricetta *insert_after = ricerca_pseudo_binaria(nome_ricetta);
    ricetta *current = head_ricetta;
    ricetta *prev = NULL;

    if ((insert_after != NULL) && (strcmp(insert_after->name, nome_ricetta) == 0)) {
        fwrite_unlocked(ignorato, 1, 9, stdout);
        return;
    }

    ricetta* nuova_ricetta = malloc(sizeof(struct ricetta)); //alloco memoria
    if (nuova_ricetta == NULL) {
        fprintf(stderr, "Failed to allocate memory for new recipe.\n");
        return;
    }
    strcpy(nuova_ricetta->name, nome_ricetta);
    nuova_ricetta->ingredienti = NULL;
    nuova_ricetta->prev = prev;
    nuova_ricetta->next = current;



    // Processa i token degli ingredienti
    ingrediente_ricetta *ultimo_ingrediente = NULL;
    for (int i = 0; token_ingredienti[i] != NULL; i += 2) {
        ingrediente_ricetta *nuovo_ingrediente = malloc(sizeof(ingrediente_ricetta));
        if (nuovo_ingrediente == NULL) {
            fprintf(stderr, "Errore: Allocazione della memoria fallita per gli ingredienti\n");
            // Libera la memoria allocata in precedenza per la ricetta
            free(nuova_ricetta);
            return;
        }
        nuovo_ingrediente->qta = atoi(token_ingredienti[i + 1]);
        nuovo_ingrediente->next = NULL;
        nuova_ricetta->total_qta += nuovo_ingrediente->qta;  // Aggiunge la quantità al totale


        // Collego la ricetta al magazzino
        magazzino * ingrediente_magazzino = punt_ingrediente(token_ingredienti[i]);
        /* TODO importante sta roba è buggata
        if ((ingrediente_magazzino != NULL) && (strcmp(ingrediente_magazzino->ingr_name, token_ingredienti[i]) == 0)) {
            nuovo_ingrediente->ingr = ingrediente_magazzino;
        }
        */
        nuovo_ingrediente->ingr = ingrediente_magazzino;

        // TODO urgente cosa fa questa parte? mi sono dimenticato.
        if (ultimo_ingrediente == NULL) {
            nuova_ricetta->ingredienti = nuovo_ingrediente;
        } else {
            ultimo_ingrediente->next = nuovo_ingrediente;
        }
        ultimo_ingrediente = nuovo_ingrediente;
    }

    // Inserisce la ricetta al posto corretto
    if (insert_after == NULL) {
        nuova_ricetta->next = head_ricetta;
        if (head_ricetta != NULL) {
            (head_ricetta)->prev = nuova_ricetta;
        }
        head_ricetta = nuova_ricetta;
    } else {
        nuova_ricetta->next = insert_after->next;
        nuova_ricetta->prev = insert_after;
        if (insert_after->next != NULL) {
            insert_after->next->prev = nuova_ricetta;
        }
        insert_after->next = nuova_ricetta;
    }

    ricette_totali += 1;
    fwrite_unlocked(aggiunto, 1, 9, stdout);
}

void aggiungi_ordine(const char *nome_ricetta, const int quantita, const int t) {

    // Ricerca della ricetta nella lista ordinata
    ricetta *ricetta_corrente = ricerca_pseudo_binaria(nome_ricetta);

    // Verifica se la ricetta è stata trovata e corrisponde esattamente
    if ((ricetta_corrente == NULL) || (strcmp(ricetta_corrente->name, nome_ricetta) != 0)) {
        fwrite_unlocked(rifiutato, 1, 10, stdout);
        return;
    }

    // Creazione di un nuovo ordine
    ordini *nuovo_ordine = malloc(sizeof(ordini));
    if (nuovo_ordine == NULL) {
        fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ordine\n");
        return;
    }

    nuovo_ordine->qta = quantita;
    nuovo_ordine->ricetta_ord = ricetta_corrente;
    nuovo_ordine->next = NULL;
    nuovo_ordine->time_placed = t;


    // Appendo alla fine della lista. Se è vuota diventa anche l'inizio.
    if (tail_ordine == NULL) {
        head_ordine = nuovo_ordine;
        tail_ordine = nuovo_ordine;
    } else {
        tail_ordine->next =nuovo_ordine;
        tail_ordine = nuovo_ordine;
    }

    // Posto dove incrementare il conto degli ordini nella ricetta
    ricetta_corrente->n_ord++;

    fwrite_unlocked(accettato, 1, 10, stdout);
}

void prepara_ordini(const int current_time) { //TODO professore: è rilevante il fatto che devo fare immediatamente la analisi?
    ordini *current_ordine = head_ordine;
    ordini *prev_ordine = NULL;

    while (current_ordine != NULL) {
        if (current_ordine->ricetta_ord == NULL) {
            printf("FATAL: ricetta_ord pointer is NULL for order placet at time %d.\n", current_ordine->time_placed);
            return; // Skip processing this order
        }
        if ((current_time != -1) && (current_ordine->time_placed > current_time)) {
            break;  // If current_time is specified and order's time exceeds it, stop processing.
        }
        ricetta *current_ricetta = current_ordine->ricetta_ord;
        ingrediente_ricetta *ingrediente_ricetta_ptr = current_ricetta->ingredienti;

        int can_fulfill = 1;  // Supponiamo di poter soddisfare l'ordine

        // Controlla se ci sono abbastanza ingredienti nel magazzino
        while (ingrediente_ricetta_ptr != NULL) {


            int needed_quantity = (ingrediente_ricetta_ptr->qta) * (current_ordine->qta);



            magazzino * magazzino_ptr = ingrediente_ricetta_ptr->ingr;

            if (magazzino_ptr->tot_av < needed_quantity) {
                can_fulfill = 0;  // Non ci sono abbastanza ingredienti
                break;
            }

            ingrediente_ricetta_ptr = ingrediente_ricetta_ptr->next;
        }

        if (can_fulfill) {

            // Rimuove gli ingredienti usati dal magazzino
            ingrediente_ricetta_ptr = current_ricetta->ingredienti;

            while (ingrediente_ricetta_ptr != NULL) {
                int needed_quantity = ingrediente_ricetta_ptr->qta * current_ordine->qta;

                // Cerca l'ingrediente nel magazzino
                magazzino* magazzino_ptr = ingrediente_ricetta_ptr->ingr;

                if (magazzino_ptr != NULL) {

                    ingrediente *ingrediente_ptr = magazzino_ptr->ingredienti;

                    while ((ingrediente_ptr != NULL) && needed_quantity > 0) {
                        if (ingrediente_ptr->qta <= needed_quantity) {
                            needed_quantity -= ingrediente_ptr->qta;

                            // Removing ingredient from list
                            magazzino_ptr->tot_av -= ingrediente_ptr->qta;
                            ingrediente *to_remove = ingrediente_ptr;
                            ingrediente_ptr = ingrediente_ptr->next;

                            magazzino_ptr->ingredienti = ingrediente_ptr;


                            free(to_remove);
                        } else {
                            magazzino_ptr->tot_av -= needed_quantity;
                            ingrediente_ptr->qta -= needed_quantity;
                            needed_quantity = 0;
                            //ingrediente_prev = ingrediente_ptr;
                            //ingrediente_ptr = ingrediente_ptr->next;
                        }

                    }
                }

                ingrediente_ricetta_ptr = ingrediente_ricetta_ptr->next;

            }

            // Rimuove l'ordine dalla lista ordini
            if (prev_ordine == NULL) {
                head_ordine = current_ordine->next;

            } else {
                prev_ordine->next = current_ordine->next;
            }

            // Aggiunge l'ordine completato alla lista ordini_completi
            ordini_completi *new_ordine_completo = malloc(sizeof(ordini_completi));
            if (new_ordine_completo == NULL) {
                fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ordine completo\n");
                return;
            }

            new_ordine_completo->ricetta_ord=current_ordine->ricetta_ord;
            new_ordine_completo->qta = current_ordine->qta;
            new_ordine_completo->dim_tot = current_ricetta->total_qta * current_ordine->qta;
            new_ordine_completo->time_placed = current_ordine->time_placed;
            new_ordine_completo->next = NULL;


            if ((head_ordine_completi == NULL) || (head_ordine_completi)->time_placed >= new_ordine_completo->time_placed) {
                new_ordine_completo->next = head_ordine_completi;
                head_ordine_completi = new_ordine_completo;
            } else {
                ordini_completi *temp = head_ordine_completi;
                while (temp->next && temp->next->time_placed < new_ordine_completo->time_placed) {
                    temp = temp->next;
                }
                new_ordine_completo->next = temp->next;
                temp->next = new_ordine_completo;
            }

            if (prev_ordine == NULL) {
                head_ordine = current_ordine->next;
            } else {
                prev_ordine->next = current_ordine->next;
            }

            free(current_ordine);
            if(tail_ordine == current_ordine){tail_ordine = prev_ordine;}
            if (prev_ordine != NULL) {
                current_ordine = prev_ordine->next; // Move to the next order if there is a previous order
            } else {
                current_ordine = head_ordine; // If there was no previous order, start from the head again
            }
        }
        else {
             prev_ordine = current_ordine;
            //fprintf(stderr, "%p %p\n", (void *)current_ordine, (void *)current_ordine->next);
            current_ordine = current_ordine->next;
        }
    }
}

void rifornisci(char *buffer, const int t) {
    char *token = strtok(buffer + 13, " \t\n");
    char *tokens[MAX_LINE_LENGTH];
    int idx = 0;

    while (token != NULL) {
        tokens[idx] = token;
        idx += 1;
        token = strtok(NULL, " \t\n");
    }
    tokens[idx] = NULL;

    for (int i = 0; tokens[i] != NULL; i += 3) {
        char *ingr_name = tokens[i];
        int qta = atoi(tokens[i + 1]);
        int expiry = atoi(tokens[i + 2]);

        magazzino* current = punt_ingrediente(ingr_name);
        if(current == NULL){fprintf(stderr, "FATAL ERROR - 04"); return;}

        // Inserisce i lotti di ingredienti in ordine di scadenza
        ingrediente *ingr_ptr = current->ingredienti, *ingr_prev = NULL;
        while (ingr_ptr != NULL && ingr_ptr->expiry < expiry) {
            ingr_prev = ingr_ptr;
            ingr_ptr = ingr_ptr->next;
        }

        // Se c'è già un lotto con la stessa scadenza, ne aumenta la quantità
        if (ingr_ptr != NULL && ingr_ptr->expiry == expiry) {

            current->tot_av +=qta;
            ingr_ptr->qta += qta;
        }
        // Altrimenti, crea un nuovo batch
        else {
            ingrediente *new_ingrediente = malloc(sizeof(ingrediente));
            if (new_ingrediente == NULL) {
                fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ingrediente\n");
                return;
            }

            current->tot_av +=qta;
            new_ingrediente->qta = qta;
            new_ingrediente->expiry = expiry;

            if (ingr_prev == NULL) {
                new_ingrediente->next = current->ingredienti;
                current->ingredienti = new_ingrediente;
            }
            else {
                new_ingrediente->next = ingr_prev->next;
                ingr_prev->next = new_ingrediente;
            }
        }
    }
    printf("rifornito\n");
}

void verifica_scadenze(const int t) { //todo questa funzione andrebbe riscritta per chiarezza
    magazzino *current = head_magazzino;

    while (current) {
        ingrediente **cur = &(current->ingredienti);
        while (*cur) {
            if ((*cur)->expiry <= t) { // todo minore o (minore o uguale) ??
                current->tot_av -=(*cur)->qta;
                ingrediente *expired = *cur;
                *cur = (*cur)->next;
                free(expired);
            }
            else {break;}
        }
        current = current->next;
    }

}

void rimuovi_ricetta(const char* nome_ricetta) {

    // Cerca la ricetta
    ricetta* da_rimuovere = ricerca_pseudo_binaria(nome_ricetta);
    if(da_rimuovere== NULL) {
        fwrite_unlocked(non_presente, 1, 13, stdout);
        return;
    }

    if(strcmp(nome_ricetta, da_rimuovere->name) !=0) {
        fwrite_unlocked(non_presente, 1, 13, stdout);
        return;
    }

    // Controlla se ci sono ordini presenti
    else if (da_rimuovere->n_ord != 0) {
        //fprintf(stderr, "Eliminazione ricetta %s, if superato\n", nome_ricetta);
        fwrite_unlocked(ordinisospeso, 1, 18, stdout);
        //fprintf(stderr, "ordini in sospeso: %s ricetta attuale: %s\n", nome_ricetta, current->name);
        return;
    }

    // Rimuove la ricetta dalla lista
    if (da_rimuovere->prev != NULL) {
        da_rimuovere->prev->next = da_rimuovere->next;
    } else {
        head_ricetta = da_rimuovere->next;
    }

    if (da_rimuovere->next != NULL) {
        da_rimuovere->next->prev = da_rimuovere->prev;
    }

    // Libera la memoria degli ingredienti
    ingrediente_ricetta *ing = da_rimuovere->ingredienti;
    while (ing != NULL) {
        ingrediente_ricetta *temp = ing;
        ing = ing->next;
        free(temp);
    }
    free(da_rimuovere);
    //fprintf(stderr, "ricetta rimossa: %s\n", nome_ricetta);
    fwrite_unlocked(rimosso, 1, 8, stdout);
}

void carica_furgone(const int max_cargo, int tempo) {
    ordini_completi *current = head_ordine_completi;

    int current_cargo = 0;
    if (current == NULL) {
        printf("camioncino vuoto\n");
        return;
    }
    while (current != NULL) {
        if ((current_cargo + current->dim_tot) > max_cargo) {
            break; // Stop if adding this order exceeds max cargo capacity
        }

        // Create new in-carico order
        ordini_in_carico *new_in_carico = malloc(sizeof(ordini_in_carico));
        if (new_in_carico == NULL) {
            fprintf(stderr, "Memory allocation failed for new in-carico order\n");
            return; // Early exit on memory allocation failure
        }
        current->ricetta_ord->n_ord--;
        strcpy(new_in_carico->name, current->ricetta_ord->name);
        new_in_carico->qta = current->qta;
        new_in_carico->dim_tot = current->dim_tot;
        new_in_carico->time_placed = current->time_placed;
        new_in_carico->next = NULL;

        // Insert new order into ordini_in_carico, sorted by dim_tot (descending) and then by time_placed (ascending for same dim_tot)
        if (head_ordine_in_carico == NULL ||
            (head_ordine_in_carico)->dim_tot < new_in_carico->dim_tot || // Check for larger dim_tot to be at the front
            ((head_ordine_in_carico)->dim_tot == new_in_carico->dim_tot && (head_ordine_in_carico)->time_placed > new_in_carico->time_placed)) { // Earlier time_placed should come first for the same dim_tot
            // Insert at the head if it's the largest or equally large but earlier
            new_in_carico->next = head_ordine_in_carico;
            head_ordine_in_carico = new_in_carico;
            } else {
                // Find the correct position to insert
                ordini_in_carico *curr_in_carico = head_ordine_in_carico, *prev_in_carico = NULL;
                while (curr_in_carico != NULL &&
                        (curr_in_carico->dim_tot > new_in_carico->dim_tot || // Continue if current item is larger
                        (curr_in_carico->dim_tot == new_in_carico->dim_tot && curr_in_carico->time_placed < new_in_carico->time_placed))) { // Continue if the current time is later
                    prev_in_carico = curr_in_carico;
                    curr_in_carico = curr_in_carico->next;
        }
                // Insert new item before the first item that has either a smaller dim_tot or the same dim_tot but a later time_placed
                new_in_carico->next = curr_in_carico;
                if (prev_in_carico == NULL) {
                    // This means new_in_carico should be the new head either because it's larger or equally large but earlier than any current orders
                    new_in_carico->next = head_ordine_in_carico;
                    head_ordine_in_carico = new_in_carico;
                } else {
                    // Insert the new order in the found spot
                    new_in_carico->next = curr_in_carico;
                    prev_in_carico->next = new_in_carico;
                }
            }


        // stampa dettagli ordini

        current_cargo += current->dim_tot;
        ordini_completi *to_remove = current;
        current = current->next;
        free(to_remove);
    }

    // Print and free ordini_in_carico list

    ordini_in_carico *print_current = head_ordine_in_carico;
    while (print_current != NULL) {
        printf("%d %s %d\n", print_current->time_placed, print_current->name, print_current->qta);
        ordini_in_carico *to_free = print_current;
        print_current = print_current->next;
        free(to_free);
    }
    head_ordine_in_carico = NULL; // Reset head_in_carico list
    head_ordine_completi = current;
}

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0'; // Replace newline with null terminator
    }
}

int read_line_unlocked(char *buffer, int max_size) {
    int act_max_size = max_size*sizeof(char);
    int i = 0;
    char ch;
    while (1) {
        if((ch = getchar_unlocked()) == EOF){break;}
        if((ch == '\n')){break;}
        if((i >= act_max_size-1)){break;}
        buffer[i] = ch;
        i+=1;
    }

    buffer[i] = '\0'; // Null-terminate the string

    return i; // Return the number of characters read, not including the null terminator
}

ricetta* ricerca_pseudo_binaria(const char* nome_ricetta) {
    ricetta *current = head_ricetta;
    ricetta *previous = NULL;
    int skip = ricette_totali / 2;  // Initially set skip to half of the list size

    while (current != NULL && skip > 3) {
        ricetta *scanner = current;
        int step = 0;

        // Attempt to skip forward in the list
        while (step < skip && scanner != NULL) {
            previous = scanner;
            scanner = scanner->next;
            step++;
        }

        // Check position of scanner in relation to target
        if (scanner == NULL || strcmp(scanner->name, nome_ricetta) >= 0) {
            skip /= 2;  // Halve the skip size
        } else {
            // Move the current forward since target is further
            current = scanner;
        }
    }

    // If skip becomes zero, do linear search for exact position
    while (current != NULL && strcmp(current->name, nome_ricetta) < 0) {
        previous = current;
        current = current->next;
    }

    if (current == NULL || strcmp(current->name, nome_ricetta) > 0) {
        return previous;  // Insert after previous
    }
    return current;  // Exact match found
}

magazzino* ricerca_pseudo_binaria_ingr(const char* nome_ingr) {
    magazzino *current = head_magazzino;
    magazzino *previous = NULL;
    int skip = ingredienti_totali / 2;  // Initially set skip to half of the list size

    while (current != NULL && skip > 3) { // todo importante skip è da valutare
        magazzino *scanner = current;
        int step = 0;

        // Attempt to skip forward in the list
        while ((step < skip) && (scanner != NULL)) {
            previous = scanner;
            scanner = scanner->next;
            step++;
        }

        // Check position of scanner in relation to target
        if ((scanner == NULL) || strcmp(scanner->ingr_name, nome_ingr) >= 0) {
            skip /= 2;  // Halve the skip size
        } else {
            // Move the current forward since target is further
            current = scanner;
        }
    }

    // If skip becomes zero, do linear search for exact position
    while ((current != NULL) && strcmp(current->ingr_name, nome_ingr) < 0) {
        previous = current;
        current = current->next;
    }

    if (current == NULL || strcmp(current->ingr_name, nome_ingr) > 0) {
        return previous;  // Insert after previous
    }
    return current;  // Exact match found
}

magazzino* punt_ingrediente(const char* nome_ingr){
    // Trovo la posizione corretta. La funzione restituisce la posizione in cui si trova l'ingrediente oppure la posizione precedente, se non esiste. Se è NULL la lista è vuota
    magazzino* pointer = ricerca_pseudo_binaria_ingr(nome_ingr);

    // Verifico se l'ingrediente esiste già, altrimenti lo creo e lo inserisco nella posizione corretta

    // Se la lista è vuota, creo un ingrediente e lo uso come primo della lista
    if(pointer == NULL) {
        magazzino* nuovo_ingrediente = malloc(sizeof(magazzino));
                    if (nuovo_ingrediente == NULL) {
                        fprintf(stderr, "FATAL ERROR - 03");
                        return NULL;
                    }

        if (head_magazzino == NULL) {
            head_magazzino = nuovo_ingrediente;
            strcpy(nuovo_ingrediente->ingr_name,nome_ingr);
            }
        else {
            nuovo_ingrediente->next = head_magazzino;
            head_magazzino = nuovo_ingrediente;
            strcpy(nuovo_ingrediente->ingr_name,nome_ingr);
            }
        ingredienti_totali += 1;
        return nuovo_ingrediente;
    }

    // Se è stato trovato l'ingrediente:
    else if (strcmp(nome_ingr, pointer->ingr_name) == 0){return pointer;}

    // altrimenti lo creo e lo inserisco al posto giusto
    else {
        magazzino* nuovo_ingrediente = malloc(sizeof(magazzino));
        if (nuovo_ingrediente == NULL) {
            fprintf(stderr, "FATAL ERROR - 02");
            return NULL;
        }
        nuovo_ingrediente->next = pointer->next;
        pointer->next = nuovo_ingrediente;
        strcpy(nuovo_ingrediente->ingr_name,nome_ingr);
        ingredienti_totali += 1;
        return nuovo_ingrediente;
    }
}
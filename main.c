// Filippo Piazza
// 2024
#define MAX_WORD_LENGTH 255
#define MAX_LINE_LENGTH 65536
#define _VERBOSE  1

#include<stdio.h>
#include<string.h>
#include<stdlib.h>


typedef struct magazzino
{
    struct magazzino *prev;
    char ingr_name[MAX_WORD_LENGTH];
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
    char nome[MAX_WORD_LENGTH];
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
    char name[MAX_WORD_LENGTH];
    int qta;
    struct ricetta* ricetta_ord;
    struct ordini *next;
    int time_placed;
} ordini;

typedef struct ordini_completi
{
    char name[MAX_WORD_LENGTH];
    int qta;
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

void aggiungi_ricetta(ricetta** head, const char* nome_ricetta, char** token_ingredienti);
void aggiungi_ordine(ordini **head_ordine, ricetta *head_ricetta, const char *nome_ricetta, const int quantita, const int t);
void prepara_ordini(magazzino **head_magazzino, ricetta *head_ricetta, ordini **head_ordine, ordini_completi **head_ordine_completi, const int current_time);
void rifornisci(char *buffer, magazzino **head_magazzino, ricetta *head_ricetta, ordini **head_ordine, ordini_completi **head_ordine_completi, const int t);
void verifica_scadenze(const int t, magazzino **head);
void rimuovi_ricetta(ricetta** head, const char* nome_ricetta);
void carica_furgone(ordini_completi **head_completi, ordini_in_carico **head_in_carico, const int max_cargo, int tempo);

void trim_newline(char *str);



void aggiungi_ricetta(ricetta** head, const char* nome_ricetta, char** token_ingredienti) {
    ricetta *current = *head;

    while ((current != NULL) && (strcmp((current)->name, nome_ricetta) < 0) ) { // verifico l'ordine
    current = (current)->next;
    }

    if ((current != NULL) && (strcmp((current)->name, nome_ricetta) == 0) ) { // controllo se esiste già una ricetta con lo stesso nome
        printf("ignorato\n");
        return;
    }

    ricetta* nuova_ricetta = malloc(sizeof(struct ricetta)); //alloco memoria
    if (nuova_ricetta == NULL) {
        fprintf(stderr, "Failed to allocate memory for new recipe.\n");
        return;
    }
    strcpy(nuova_ricetta->name, nome_ricetta);
    nuova_ricetta->ingredienti = NULL;
    nuova_ricetta->prev = nuova_ricetta->next = NULL;
    nuova_ricetta->total_qta = 0;  // Inizializza la quantità totale a zero

    // Processa i token degli ingredienti todo potrebbe aver senso linkare direttamente il nodo degli ingredienti nel magazzino
    ingrediente_ricetta *ultimo_ingrediente = NULL;
    for (int i = 1; token_ingredienti[i] != NULL; i += 2) {
        ingrediente_ricetta *nuovo_ingrediente = malloc(sizeof(ingrediente_ricetta));
        if (nuovo_ingrediente == NULL) {
            fprintf(stderr, "Errore: Allocazione della memoria fallita per gli ingredienti\n");
            // Libera la memoria allocata in precedenza per la ricetta
            free(nuova_ricetta);
            return;
        }
        strcpy(nuovo_ingrediente->nome, token_ingredienti[i]);
        nuovo_ingrediente->qta = atoi(token_ingredienti[i + 1]);
        nuovo_ingrediente->next = NULL;
        nuova_ricetta->total_qta += nuovo_ingrediente->qta;  // Aggiunge la quantità al totale
        nuova_ricetta->n_ord = 0;

        if (ultimo_ingrediente == NULL) {
            nuova_ricetta->ingredienti = nuovo_ingrediente;
        } else {
            ultimo_ingrediente->next = nuovo_ingrediente;
        }
        ultimo_ingrediente = nuovo_ingrediente;
    }

    // Inserisce la nuova ricetta nella posizione corretta
    if (current == *head) {
        // Inserimento all'inizio della lista
        nuova_ricetta->next = *head;
        if (*head != NULL) {
            (*head)->prev = nuova_ricetta;
        }
        *head = nuova_ricetta;
    } else {
        // Inserimento nel mezzo o alla fine della lista
        nuova_ricetta->next = current;
        if (current != NULL) {
            nuova_ricetta->prev = current->prev;
            current->prev = nuova_ricetta;
        }
        if (nuova_ricetta->prev != NULL) {
            nuova_ricetta->prev->next = nuova_ricetta;
        }
    }

    printf("aggiunta\n");
}

void aggiungi_ordine(ordini **head_ordine, ricetta *head_ricetta, const char *nome_ricetta, const int quantita, const int t) {
    // Ricerca della ricetta nella lista ordinata
    ricetta *ricetta_corrente = head_ricetta;
    while (ricetta_corrente != NULL && strcmp(ricetta_corrente->name, nome_ricetta) < 0) {
        ricetta_corrente = ricetta_corrente->next;
    }

    // Verifica se la ricetta è stata trovata e corrisponde esattamente
    if (ricetta_corrente == NULL || strcmp(ricetta_corrente->name, nome_ricetta) != 0) {
        printf("rifiutato\n");
        return;
    }

    // Creazione di un nuovo ordine
    ordini *nuovo_ordine = malloc(sizeof(ordini));
    if (nuovo_ordine == NULL) {
        fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ordine\n");
        return;
    }

    strcpy(nuovo_ordine->name, nome_ricetta);
    nuovo_ordine->qta = quantita;
    nuovo_ordine->ricetta_ord = ricetta_corrente;
    nuovo_ordine->next = NULL;
    nuovo_ordine->time_placed = t;


    // Append to the end of the list
    if (*head_ordine == NULL) {
        *head_ordine = nuovo_ordine; // The list is empty, new order becomes the head
    } else {
        ordini *last_ordine = *head_ordine;
        while (last_ordine->next != NULL) { // Traverse to the end of the list
            last_ordine = last_ordine->next;
        }
        last_ordine->next = nuovo_ordine; // Append the new order
    }

    // Posto dove incrementare il conto degli ordini nella ricetta
    ricetta_corrente->n_ord++;

    printf("accettato\n");
}

void prepara_ordini(magazzino **head_magazzino, ricetta *head_ricetta, ordini **head_ordine, ordini_completi **head_ordine_completi, const int current_time) {// Check if head_magazzino is NULL
    if (!head_magazzino) {
        //printf("head_magazzino is NULL.\n");
        return; // Early exit if head_magazzino is NULL
    }

    // Check if the object head_magazzino points to is NULL
    if (!*head_magazzino) {
        //fprintf(stderr,"*head_magazzino is NULL.\n");
        return; // Early exit if the object pointed to by head_magazzino is NULL
    }

    // Check if head_ordine is NULL
    if (!head_ordine) {
        //fprintf(stderr,"head_ordine is NULL.\n");
        return; // Early exit if head_ordine is NULL
    }

    // Check if the object head_ordine points to is NULL
    if (!*head_ordine) {
        //fprintf(stderr,"*head_ordine is NULL.\n");
        return; // Early exit if the object pointed to by head_ordine is NULL
    }

    ordini *current_ordine = *head_ordine;
    ordini *prev_ordine = NULL;

    while (current_ordine != NULL) {
        if (!current_ordine->ricetta_ord) {
            printf("ricetta_ord pointer is NULL for order %s.\n", current_ordine->name);
            current_ordine = current_ordine->next;
            continue; // Skip processing this order
        }
        if (current_time != -1 && current_ordine->time_placed > current_time) {
            break;  // If current_time is specified and order's time exceeds it, stop processing.
        }
        ricetta *current_ricetta = current_ordine->ricetta_ord;
        ingrediente_ricetta *ingrediente_ricetta_ptr = current_ricetta->ingredienti;

        int can_fulfill = 1;  // Supponiamo di poter soddisfare l'ordine

        // Controlla se ci sono abbastanza ingredienti nel magazzino
        while (ingrediente_ricetta_ptr != NULL) {
            magazzino *magazzino_ptr = *head_magazzino;

            int needed_quantity = ingrediente_ricetta_ptr->qta * current_ordine->qta;
            int found_quantity = 0;

            // Cerca l'ingrediente nel magazzino
            while (magazzino_ptr != NULL && strcmp(magazzino_ptr->ingr_name, ingrediente_ricetta_ptr->nome) != 0) {
                magazzino_ptr = magazzino_ptr->next;
            }

            // Raggruppa lotti con lo stesso ingrediente
            if (magazzino_ptr != NULL) {
                ingrediente *ingrediente_ptr = magazzino_ptr->ingredienti;

                while (ingrediente_ptr != NULL && found_quantity < needed_quantity) {
                    found_quantity += ingrediente_ptr->qta;
                    ingrediente_ptr = ingrediente_ptr->next;
                }
            }
            // fprintf(stderr, "tempo %d qta ingrediente %s per ricetta %s %d\n",current_time, ingrediente_ricetta_ptr-> nome, current_ricetta->name, found_quantity);
            if (found_quantity < needed_quantity) {
                can_fulfill = 0;  // Non ci sono abbastanza ingredienti
                // fprintf(stderr, "non abbastanza ingredienti per ricetta %s: found %d needed %d\n", current_ricetta->name, found_quantity, needed_quantity);
                break;
            }

            ingrediente_ricetta_ptr = ingrediente_ricetta_ptr->next;
        }

        //fprintf(stderr, "now onto can fulfill %d \n", can_fulfill);
        if (can_fulfill) {
            current_ricetta->n_ord--;
            // Rimuove gli ingredienti usati dal magazzino
            ingrediente_ricetta_ptr = current_ricetta->ingredienti;

            while (ingrediente_ricetta_ptr != NULL) {
                magazzino *magazzino_ptr = *head_magazzino;

                int needed_quantity = ingrediente_ricetta_ptr->qta * current_ordine->qta;


                // Cerca l'ingrediente nel magazzino
                while (magazzino_ptr != NULL && strcmp(magazzino_ptr->ingr_name, ingrediente_ricetta_ptr->nome) != 0) {

                    magazzino_ptr = magazzino_ptr->next;
                }

                if (magazzino_ptr != NULL) {

                    ingrediente *ingrediente_ptr = magazzino_ptr->ingredienti;
                    ingrediente *ingrediente_prev = NULL;

                    while ((ingrediente_ptr != NULL) && needed_quantity > 0) {
                        //fprintf(stderr, "sane serbia %p %d       ", (void *)ingrediente_ptr, needed_quantity);
                        if (ingrediente_ptr->qta <= needed_quantity) {
                            needed_quantity -= ingrediente_ptr->qta;  // Decrease needed by available

                            // Removing ingredient from list
                            ingrediente *to_remove = ingrediente_ptr;
                            ingrediente_ptr = ingrediente_ptr->next;  // Move to next ingredient before removing

                            if (ingrediente_prev == NULL) {
                                magazzino_ptr->ingredienti = ingrediente_ptr;  // Update head if first ingredient
                            } else {
                                ingrediente_prev->next = ingrediente_ptr;  // Link previous to next
                            }

                            free(to_remove);  // Free the removed ingredient
                        } else {
                            ingrediente_ptr->qta -= needed_quantity;  // Reduce quantity of current ingredient
                            needed_quantity = 0;  // Set needed to zero as we've found enough
                            //ingrediente_prev = ingrediente_ptr;
                            //ingrediente_ptr = ingrediente_ptr->next;
                        }

                    }
                }

                ingrediente_ricetta_ptr = ingrediente_ricetta_ptr->next;

            }

            // Rimuove l'ordine dalla lista ordini
            if (prev_ordine == NULL) {
                *head_ordine = current_ordine->next;

            } else {
                prev_ordine->next = current_ordine->next;
            }

            // Aggiunge l'ordine completato alla lista ordini_completi
            ordini_completi *new_ordine_completo = malloc(sizeof(ordini_completi));
            if (new_ordine_completo == NULL) {
                fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ordine completo\n");
                return;
            }

            strcpy(new_ordine_completo->name, current_ordine->name);
            new_ordine_completo->qta = current_ordine->qta;
            new_ordine_completo->dim_tot = current_ricetta->total_qta * current_ordine->qta;
            new_ordine_completo->time_placed = current_ordine->time_placed;
            new_ordine_completo->next = NULL;


            if (!*head_ordine_completi || (*head_ordine_completi)->time_placed >= new_ordine_completo->time_placed) {
                new_ordine_completo->next = *head_ordine_completi;
                *head_ordine_completi = new_ordine_completo;
            } else {
                ordini_completi *temp = *head_ordine_completi;
                while (temp->next && temp->next->time_placed < new_ordine_completo->time_placed) {
                    temp = temp->next;
                }
                new_ordine_completo->next = temp->next;
                temp->next = new_ordine_completo;
            }

            if (prev_ordine == NULL) {
                *head_ordine = current_ordine->next;
            } else {
                prev_ordine->next = current_ordine->next;
            }

            free(current_ordine);
            if (prev_ordine != NULL) {
                current_ordine = prev_ordine->next; // Move to the next order if there is a previous order
            } else {
                current_ordine = *head_ordine; // If there was no previous order, start from the head again
            }
        }
        else {
             prev_ordine = current_ordine;
            //fprintf(stderr, "%p %p\n", (void *)current_ordine, (void *)current_ordine->next);
            current_ordine = current_ordine->next;
        }
    }
}

void rifornisci(char *buffer, magazzino **head_magazzino, ricetta *head_ricetta, ordini **head_ordine, ordini_completi **head_ordine_completi,const int t) {
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

        magazzino *current = *head_magazzino, *prev = NULL;
        while (current != NULL && strcmp(current->ingr_name, ingr_name) < 0) {
            prev = current;
            current = current->next;
        }

        if (current != NULL && strcmp(current->ingr_name, ingr_name) == 0) {
            // Inserisce gli ingredienti in ordine di scadenza
            ingrediente *ingr_ptr = current->ingredienti, *ingr_prev = NULL;
            while (ingr_ptr != NULL && ingr_ptr->expiry < expiry) {
                ingr_prev = ingr_ptr;
                ingr_ptr = ingr_ptr->next;
            }
            if (ingr_ptr != NULL && ingr_ptr->expiry == expiry) {
                // Se c'è già un lotto con la stessa scadenza, aggiunge la nuova quantità
                ingr_ptr->qta += qta;
            } else {
                // altrimenti aggiunge un nuovo lotto
                ingrediente *new_ingrediente = malloc(sizeof(ingrediente));
                if (new_ingrediente == NULL) {
                    fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ingrediente\n");
                    return;
                }
                new_ingrediente->qta = qta;
                new_ingrediente->expiry = expiry;

                if (ingr_prev == NULL) {
                    new_ingrediente->next = current->ingredienti;
                    current->ingredienti = new_ingrediente;
                } else {
                    new_ingrediente->next = ingr_prev->next;
                    ingr_prev->next = new_ingrediente;
                }
            }
        } else {
            // Se non c'é già un ingrediente con lo stesso nome, lo crea
            magazzino *new_node = malloc(sizeof(magazzino));
            if (new_node == NULL) {
                fprintf(stderr, "Errore: Allocazione della memoria fallita per il magazzino\n");
                return;
            }
            strcpy(new_node->ingr_name, ingr_name);
            new_node->prev = new_node->next = NULL;
            new_node->ingredienti = malloc(sizeof(ingrediente));
            if (new_node->ingredienti == NULL) {
                fprintf(stderr, "Errore: Allocazione della memoria fallita per l'ingrediente\n");
                free(new_node);
                return;
            }
            new_node->ingredienti->qta = qta;
            new_node->ingredienti->expiry = expiry;
            new_node->ingredienti->next = NULL;

            if (prev == NULL) {
                new_node->next = *head_magazzino;
                if (*head_magazzino != NULL) (*head_magazzino)->prev = new_node;
                *head_magazzino = new_node;
            } else {
                new_node->next = prev->next;
                new_node->prev = prev;
                if (prev->next != NULL) prev->next->prev = new_node;
                prev->next = new_node;
            }
        }
    }
    printf("rifornito\n");
    prepara_ordini(head_magazzino, head_ricetta, head_ordine, head_ordine_completi, t);
}

void verifica_scadenze(const int t, magazzino **head) {
    magazzino *current = *head;

    while (current != NULL) {
        ingrediente *ingr_ptr = current->ingredienti;
        ingrediente *ingr_prev = NULL;

        // Check for expired lots and remove them
        while (ingr_ptr != NULL) {
            if (ingr_ptr->expiry <= t) {
                ingrediente *to_remove = ingr_ptr;
                if (ingr_prev == NULL) {
                    current->ingredienti = ingr_ptr->next;
                } else {
                    ingr_prev->next = ingr_ptr->next;
                }
                ingr_ptr = ingr_ptr->next;
                free(to_remove);
            } else {
                ingr_prev = ingr_ptr;
                ingr_ptr = ingr_ptr->next;
            }
        }

        // Move to the next magazzino node
        current = current->next;
    }
}

void rimuovi_ricetta(ricetta** head, const char* nome_ricetta) {
    ricetta *current = *head;

    // Cerca la ricetta
    while ((current != NULL) && strcmp(current->name, nome_ricetta) > 0) { //todo forse serve =!
        //fprintf(stderr, "ricetta da eliminare: %s ricetta attuale: %s\n", nome_ricetta, current->name);
        current = current->next;

    }
    //fprintf(stderr, "Eliminazione ricetta %s, while superato, current %p\n", nome_ricetta, current);
    // Controlla se la ricetta è stata trovata
    if (current == NULL) {
        printf("non presente\n");
        //fprintf(stderr, "ricetta assente: %s\n", nome_ricetta);
        return;
    }

    // Controlla se ci sono ordini presenti
    else if (current->n_ord != 0) {
        //fprintf(stderr, "Eliminazione ricetta %s, if superato\n", nome_ricetta);
        printf("ordini in sospeso\n");
        //fprintf(stderr, "ordini in sospeso: %s ricetta attuale: %s\n", nome_ricetta, current->name);
        return;
    }

    // Rimuove la ricetta dalla lista
    if (current->prev != NULL) {
        current->prev->next = current->next;
    } else {
        *head = current->next;
    }

    if (current->next != NULL) {
        current->next->prev = current->prev;
    }

    // Libera la memoria degli ingredienti
    ingrediente_ricetta *ing = current->ingredienti;
    while (ing != NULL) {
        ingrediente_ricetta *temp = ing;
        ing = ing->next;
        free(temp);
    }
    free(current);
    //fprintf(stderr, "ricetta rimossa: %s\n", nome_ricetta);
    printf("rimossa\n");
}

void carica_furgone(ordini_completi **head_completi, ordini_in_carico **head_in_carico, const int max_cargo, int tempo) {
    ordini_completi *current = *head_completi;

    int current_cargo = 0;
    if (current == NULL) {
        printf("camioncino vuoto");
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
        strcpy(new_in_carico->name, current->name);
        new_in_carico->qta = current->qta;
        new_in_carico->dim_tot = current->dim_tot;
        new_in_carico->time_placed = current->time_placed;
        new_in_carico->next = NULL;

        // Insert new order into ordini_in_carico, sorted by dim_tot (descending) and then by time_placed (ascending for same dim_tot)
        if (*head_in_carico == NULL ||
            (*head_in_carico)->dim_tot < new_in_carico->dim_tot || // Check for larger dim_tot to be at the front
            ((*head_in_carico)->dim_tot == new_in_carico->dim_tot && (*head_in_carico)->time_placed > new_in_carico->time_placed)) { // Earlier time_placed should come first for the same dim_tot
            // Insert at the head if it's the largest or equally large but earlier
            new_in_carico->next = *head_in_carico;
            *head_in_carico = new_in_carico;
            } else {
                // Find the correct position to insert
                ordini_in_carico *curr_in_carico = *head_in_carico, *prev_in_carico = NULL;
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
                    new_in_carico->next = *head_in_carico;
                    *head_in_carico = new_in_carico;
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

    ordini_in_carico *print_current = *head_in_carico;
    while (print_current != NULL) {
        printf("%d %s %d\n", print_current->time_placed, print_current->name, print_current->qta);
        ordini_in_carico *to_free = print_current;
        print_current = print_current->next;
        free(to_free);
    }
    *head_in_carico = NULL; // Reset head_in_carico list
    current_cargo = 0;
    *head_completi = current;
}

int read_line_unlocked(char *buffer, int max_size);

int main(void)
{

    char* buffer = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
    if (!buffer) {
        perror("Failed to allocate buffer");
        return EXIT_FAILURE;
    }
    // generazione liste
    ricetta* head_ricetta = NULL;
    ordini *head_ordine = NULL;
    magazzino *head_magazzino = NULL;
    ordini_completi *head_ordine_completi = NULL;
    ordini_in_carico *head_ordine_in_carico = NULL;

    int rifornimento_flag = 0;

    //input iniziale di configurazione del furgone

    if(read_line_unlocked(buffer, MAX_LINE_LENGTH) == 0){return 69420;}
    char *ptr = buffer;
    int tempocorriere = strtol(ptr, &ptr, 10);
    int max_cargo = strtol(ptr, &ptr, 10);
    //printf("%d%d\n", max_cargo, tempocorriere);
    memset(buffer, 0, MAX_LINE_LENGTH); // pulisce il buffer

    //la variabile cd_corriere funziona da countdown
    int cd_corriere = tempocorriere;


    //la variabile t conta il tempo
    int t = 0;

    while(1){
        if(_VERBOSE){fprintf(stderr, "T %d\n", t);}

        //controllo se arriva il corriere
        if (cd_corriere == 0) {
            //todo qua va implementata la logica del corriere
            if(_VERBOSE){fprintf(stderr, "Corriere...\n");}
            carica_furgone(&head_ordine_completi, &head_ordine_in_carico, max_cargo, t);
            cd_corriere = tempocorriere-1;
            if(_VERBOSE){fprintf(stderr, "OK\n");}
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
            if(_VERBOSE){fprintf(stderr, "Aggiungo ricetta...");}
            char *token = strtok(buffer + 17, " \t\n"); //verifica offset
            // *token è il nome della ricetta
            //if (token == NULL) return; \\ nome ricetta assente

            char* tokens[MAX_LINE_LENGTH]; // contiene il resto del comando
            int idx = 0;

            char *nome_ricetta = token;
            while (token != NULL && idx < (MAX_LINE_LENGTH / 2)) {
                tokens[idx] = token;
                idx += 1;
                token = strtok(NULL, " \t\n");
            }

            tokens[idx] = NULL; // aggiungo terminatore nullo alla lista dei token

            aggiungi_ricetta(&head_ricetta, nome_ricetta, tokens);
            if(_VERBOSE){fprintf(stderr, "OK\n");}
            }

        // se rimuovi_ricetta
        else if(buffer[2] == 'm'){
            if(_VERBOSE){fprintf(stderr, "Rimuovo ricetta...");}
            char *token = strtok(buffer + 16, " "); //verifica offset
            rimuovi_ricetta(&head_ricetta, token);
            if(_VERBOSE){fprintf(stderr, "OK\n");}
        }

        // se ordine
        else if(buffer[2] == 'd'){
            if(_VERBOSE){fprintf(stderr, "Aggiungo ordine...");}
            char *nome_ricetta = strtok(buffer + 7, " "); //verifica offset
            char *qta_str = strtok(NULL, " \t\n");
            if (nome_ricetta == NULL || qta_str == NULL) {
                printf("Input non valido\n");
                continue;
            }
            int quantita = atoi(qta_str);
            aggiungi_ordine(&head_ordine, head_ricetta, nome_ricetta, quantita, t);
            if(_VERBOSE){fprintf(stderr, "OK\n");}
        }

        // se rifornimento
        else if(buffer[2] == 'f'){
            if(_VERBOSE){fprintf(stderr, "Rifornimento...");}
            rifornisci(buffer, &head_magazzino, head_ricetta, &head_ordine, &head_ordine_completi, t);
            rifornimento_flag = 1;
            if(_VERBOSE){fprintf(stderr, "OK\n");}

        }

        //verifico per ogni ingrediente le cose scadute

        verifica_scadenze(t, &head_magazzino);
        if(_VERBOSE){fprintf(stderr, "Verifica scadenze ok\n");}

        if(rifornimento_flag == 0) {
            if(_VERBOSE){fprintf(stderr, "Preparo ordini...");}
            prepara_ordini(&head_magazzino, head_ricetta, &head_ordine, &head_ordine_completi, t);
            if(_VERBOSE){fprintf(stderr, "OK\n");}
        }

        t += 1;
        rifornimento_flag = 0;
        memset(buffer, 0, MAX_LINE_LENGTH); // pulisce il buffer


    }

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
        if((i >= act_max_size)){break;}
        buffer[i] = ch;
        i+=1;
    }

    buffer[i] = '\0'; // Null-terminate the string

    return i; // Return the number of characters read, not including the null terminator
}
// Filippo Piazza
// 2024
#define MAX_WORD_LENGTH 128
#define MAX_LINE_LENGTH 32768

#include <math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>


typedef struct magazzino
{
    char ingr_name[MAX_WORD_LENGTH];
    unsigned int tot_av;
    struct ingrediente *ingredienti;
    struct magazzino *left;
    struct magazzino *right;
}magazzino;

typedef struct ingrediente
{
    unsigned int qta;
    unsigned int expiry;
    struct ingrediente *next;
}ingrediente ;

typedef struct  ingrediente_ricetta
{
    magazzino *ingr;
    unsigned int qta;
    struct ingrediente_ricetta *next;
} ingrediente_ricetta;

typedef struct ricetta
{
    char name[MAX_WORD_LENGTH];
    struct ingrediente_ricetta* ingredienti; // puntatore agli ingredienti
    unsigned int n_ord;
    unsigned int total_qta;
    struct ricetta *left;
    struct ricetta *right;
} ricetta;


typedef struct ordini
{
    unsigned int qta;
    struct ricetta* ricetta_ord;
    struct ordini *next;
    unsigned int time_placed;
} ordini;

typedef struct ordini_in_carico
{
    char name[MAX_WORD_LENGTH];
    unsigned int qta;
    unsigned int dim_tot;
    unsigned int time_placed;
    struct ordini_in_carico *next;
}ordini_in_carico;

// ricette
void aggiungi_ricetta(const char* nome_ricetta, char** token_ingredienti);
ricetta* search_ricetta(const char* nome_ricetta);
ricetta* insert_ricetta(const char* nome_ricetta);
void rimuovi_ricetta(const char* nome_ricetta);
//ordini
void aggiungi_ordine(const char *nome_ricetta, const int quantita, const int t);
void prepara_ordini(const int current_time, ordini* ordine_ingresso, ordini* extail);
void carica_furgone(const int max_cargo, int tempo);
//ingredienti
void rifornisci(char *buffer, const int t);
magazzino* punt_ingrediente(const char* nome_ingr);
void verifica_scadenze(magazzino * current,const int t);
//util
void trim_newline(char *str);


//int ricette_totali = 0;

ricetta *head_ricetta = NULL;
ordini *head_ordine = NULL;
ordini *tail_ordine = NULL;
magazzino *head_magazzino = NULL;
ordini *head_ordine_completi = NULL;
ordini_in_carico *head_ordine_in_carico = NULL;

int main(void)
/*  TODO: attenzione, ogni volta che avviene un ciclo viene chiamata una malloc e allocata a 0. Per efficienza, sarebbe necessario
 *          e inizializzare la memoria solo quando effettivamente utilizzata.
 *  TODO: sarebbe utile rimuovere il buffer e leggere l'input token per token
 *  TODO: una lista ordinata che contiene il tempo e i puntatori e serve per gestire le scadenze.
 *  TODO: int hash negli alberi
 *  TODO: riduci bzero
 *  TODO: rimuovi ordini in carico per risparmiare tempo
 */

{

    char buffer[MAX_LINE_LENGTH];

    //input iniziale di configurazione del furgone
    if(fgets(buffer, sizeof(buffer), stdin) == NULL){return 69420;}
    char *ptr = buffer;
    int tempocorriere = strtol(ptr, &ptr, 10);
    int max_cargo = strtol(ptr, &ptr, 10);
    //printf("%d%d\n", max_cargo, tempocorriere);
    memset(buffer, 0, sizeof(buffer)); // pulisce il buffer

    //la variabile cd_corriere funziona da countdown
    int cd_corriere = tempocorriere;


    //la variabile t conta il tempo
    unsigned int t = 0;

    while(1){
        verifica_scadenze(head_magazzino, t);


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

        if(fgets(buffer, sizeof(buffer), stdin) == NULL){break;}
        trim_newline(buffer);


        // se aggiungi_ricetta
        if(buffer[2] == 'g'){
            char *token = strtok(buffer + 17, " \t\n"); //verifica offset

            char* tokens[MAX_LINE_LENGTH]; // contiene il resto del comando
            unsigned int idx = 0;

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
            prepara_ordini(-1, NULL, NULL);
        }

         // verifica
        //prepara_ordini(-1, NULL, NULL);
        t += 1;
        memset(buffer, 0, MAX_LINE_LENGTH); // pulisce il buffer


    }

}

void aggiungi_ricetta(const char* nome_ricetta, char** token_ingredienti) {
    // Search for the recipe; if it exists, return early
    if (search_ricetta(nome_ricetta) != NULL) {
        printf("ignorato\n");
        return;
    }

    // Insert the new recipe into the binary tree
    ricetta* nuova_ricetta = insert_ricetta(nome_ricetta);
    nuova_ricetta->ingredienti = NULL;
    nuova_ricetta->n_ord = 0;   // Initialize order count
    nuova_ricetta->total_qta = 0;  // Initialize total quantity

    // Process the tokenized ingredient list and associate them with the recipe
    ingrediente_ricetta *ultimo_ingrediente = NULL;

    for (int i = 0; token_ingredienti[i] != NULL; i += 2) {
        // Allocate a new ingredient in the recipe
        ingrediente_ricetta *nuovo_ingrediente_ricetta = malloc(sizeof(ingrediente_ricetta));
        bzero(nuovo_ingrediente_ricetta, sizeof(ingrediente_ricetta));

        // Get the quantity for the ingredient
        nuovo_ingrediente_ricetta->qta = atoi(token_ingredienti[i + 1]);
        nuova_ricetta->total_qta += nuovo_ingrediente_ricetta->qta; // Update total quantity in recipe

        // Link the recipe to the warehouse ingredient
        magazzino *ingrediente_magazzino = punt_ingrediente(token_ingredienti[i]);

        // Associate the ingredient in the warehouse with the recipe
        nuovo_ingrediente_ricetta->ingr = ingrediente_magazzino;

        // Chain the ingredients together in the recipe
        nuovo_ingrediente_ricetta->next = NULL;

        if (ultimo_ingrediente == NULL) {
            nuova_ricetta->ingredienti = nuovo_ingrediente_ricetta; // First ingredient
        } else {
            ultimo_ingrediente->next = nuovo_ingrediente_ricetta; // Subsequent ingredients
        }
        ultimo_ingrediente = nuovo_ingrediente_ricetta;
    }

    // Recipe has been added successfully
    printf("aggiunta\n");
}

//TODO Alberto -> prepara ordine subito non necessario
void aggiungi_ordine(const char *nome_ricetta, const int quantita, const int t) {
    ordini* extail = NULL;
    // Ricerca della ricetta nella lista ordinata
    ricetta *ricetta_corrente = search_ricetta(nome_ricetta);

    // Verifica se la ricetta è stata trovata e corrisponde esattamente
    if ((ricetta_corrente == NULL) || (strcmp(ricetta_corrente->name, nome_ricetta) != 0)) {
        printf("rifiutato\n");
        return;
    }

    // Creazione di un nuovo ordine
    ordini *nuovo_ordine = malloc(sizeof(ordini));
    bzero(nuovo_ordine, sizeof(ordini));

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
        extail = tail_ordine;
        tail_ordine = nuovo_ordine;
    }

    // Posto dove incrementare il conto degli ordini nella ricetta
    ricetta_corrente->n_ord++;

    printf("accettato\n");
    prepara_ordini(-1, tail_ordine, extail);
}

void prepara_ordini(const int current_time, ordini* ordine_ingresso, ordini* extail) {
    ordini *current_ordine = head_ordine;
    ordini *prev_ordine = NULL;
    if(ordine_ingresso != NULL){current_ordine = ordine_ingresso; prev_ordine = extail;}

    while (current_ordine != NULL) {
        ricetta *current_ricetta = current_ordine->ricetta_ord;
        if (current_ricetta == NULL){fprintf(stderr, "current_ricetta is null");}
        if (current_ricetta->ingredienti == NULL){fprintf(stderr, "current_ricetta->ingredienti is null");}
        ingrediente_ricetta *ingrediente_ricetta_ptr = current_ricetta->ingredienti;

        int can_fulfill = 1;  // Supponiamo di poter soddisfare l'ordine
        // Controlla se ci sono abbastanza ingredienti nel magazzino

        while (ingrediente_ricetta_ptr != NULL) {
            int needed_quantity = (ingrediente_ricetta_ptr->qta) * (current_ordine->qta);
            if (ingrediente_ricetta_ptr->ingr->tot_av < needed_quantity) {
                can_fulfill = 0;  // Non ci sono abbastanza ingredienti
                break;
            }
            // magazzino * magazzino_ptr = ingrediente_ricetta_ptr->ingr;
            ingrediente_ricetta_ptr = ingrediente_ricetta_ptr->next;
        }

        if (can_fulfill) {

            // Rimuove gli ingredienti usati dal magazzino
            ingrediente_ricetta_ptr = current_ricetta->ingredienti;

            while (ingrediente_ricetta_ptr != NULL) {
                int needed_quantity = (ingrediente_ricetta_ptr->qta) * (current_ordine->qta);
                // Cerca l'ingrediente nel magazzino
                magazzino* magazzino_ptr = ingrediente_ricetta_ptr->ingr;
                if (magazzino_ptr != NULL) {
                    ingrediente* ingrediente_ptr = magazzino_ptr->ingredienti;
                    while ((ingrediente_ptr != NULL) && (needed_quantity > 0)) {
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
            ordini *new_ordine_completo = malloc(sizeof(ordini));
            bzero(new_ordine_completo, sizeof(ordini));
            //todo importante alberto basta spostare l'ordine, non serve sto casino
            new_ordine_completo->ricetta_ord = current_ordine->ricetta_ord;
            new_ordine_completo->qta = current_ordine->qta;
            new_ordine_completo->time_placed = current_ordine->time_placed;
            new_ordine_completo->next = NULL;

            //printf("L'ordine %d è stato completato\n", current_ordine->time_placed);

            if ((head_ordine_completi == NULL) || ((head_ordine_completi)->time_placed >= new_ordine_completo->time_placed)) {
                new_ordine_completo->next = head_ordine_completi;
                head_ordine_completi = new_ordine_completo;
            } else {
                ordini *temp = head_ordine_completi;
                while ((temp->next != NULL) && (temp->next->time_placed < new_ordine_completo->time_placed)) {
                    temp = temp->next;
                }
                new_ordine_completo->next = temp->next;
                temp->next = new_ordine_completo;
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

        if(expiry <= t){continue;}

        magazzino* current = punt_ingrediente(ingr_name);

        // Inserisce i lotti di ingredienti in ordine di scadenza
        ingrediente *ingr_ptr = current->ingredienti, *ingr_prev = NULL;
        while ((ingr_ptr != NULL) && (ingr_ptr->expiry < expiry)) {
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
            bzero(new_ingrediente, sizeof(ingrediente));

            current->tot_av +=qta;
            new_ingrediente->qta = qta;
            new_ingrediente->expiry = expiry;
            new_ingrediente->next = NULL;

            //TODO Alberto -> controlla funzione aggiungiLotto
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

void verifica_scadenze(magazzino * current,const int t) {
    // Base case: if the current node is NULL, return
    if (current == NULL) {
        return;
    }

    // Process the left subtree recursively
    verifica_scadenze(current->left, t);

    // Process the current node
    ingrediente *cur = current->ingredienti;
    while (cur != NULL && cur->expiry <= t) {
        current->tot_av -= cur->qta;
        current->ingredienti = cur->next;
        free(cur);
        cur = current->ingredienti; // Move to the next ingredient
    }

    // Process the right subtree recursively
    verifica_scadenze(current->right, t);
}


//TODO Alberto -> controlla la funzione gestisciCamioncino
void carica_furgone(const int max_cargo, int tempo) {
    ordini *current = head_ordine_completi;
    int current_cargo = 0;
    if (current == NULL) {
        printf("camioncino vuoto\n");
        return;
    }
    while (current != NULL) {
        if (((current_cargo + (current->ricetta_ord->total_qta * current->qta))) > max_cargo) {
            break; // Stop if adding this order exceeds max cargo capacity
        }

        // Create new in-carico order
        ordini_in_carico *new_in_carico = malloc(sizeof(ordini_in_carico));
        bzero(new_in_carico, sizeof(ordini_in_carico));

        current->ricetta_ord->n_ord--;
        strcpy(new_in_carico->name, current->ricetta_ord->name);
        new_in_carico->qta = current->qta;
        new_in_carico->dim_tot = (current->ricetta_ord->total_qta * current->qta);
        new_in_carico->time_placed = current->time_placed;
        new_in_carico->next = NULL;

        // Insert new order into ordini_in_carico, sorted by dim_tot (descending) and then by time_placed (ascending for same dim_tot)
        if ((head_ordine_in_carico == NULL) ||
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
                       (curr_in_carico->dim_tot == new_in_carico->dim_tot && curr_in_carico->time_placed < new_in_carico->time_placed))) { // Continue if the current time is earlier
                    prev_in_carico = curr_in_carico;
                    curr_in_carico = curr_in_carico->next;
                       }
                // Insert new item before the first item that has either a smaller dim_tot or the same dim_tot but a later time_placed
                new_in_carico->next = curr_in_carico;
                if (prev_in_carico == NULL) {
                    head_ordine_in_carico = new_in_carico; // This line is corrected to set head_ordine_in_carico correctly
                } else {
                    prev_in_carico->next = new_in_carico;
                }
            }


        current_cargo += (current->ricetta_ord->total_qta * current->qta);
        ordini *to_remove = current;
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

ricetta* insert_ricetta(const char* nome_ricetta) {
    // Create a new node
    ricetta* new_node = (ricetta*)malloc(sizeof(ricetta));
    if (!new_node) {
        printf("Memory allocation error\n");
        return NULL;
    }

    // Initialize the new node's fields
    strcpy(new_node->name, nome_ricetta);
    new_node->ingredienti = NULL;
    new_node->n_ord = 0;
    new_node->total_qta = 0;
    new_node->left = NULL;
    new_node->right = NULL;

    // If the tree is empty, insert the new node as the root
    if (head_ricetta == NULL) {
        head_ricetta = new_node;
        return head_ricetta;
    }

    // Traverse the BST and find the correct insertion point
    ricetta* current = head_ricetta;
    ricetta* parent = NULL;

    while (current != NULL) {
        parent = current;
        if (strcmp(nome_ricetta, current->name) < 0) {
            current = current->left;
        } else if (strcmp(nome_ricetta, current->name) > 0) {
            current = current->right;
        } else {
            // If the recipe already exists, don't insert it
            free(new_node);
            return current;
        }
    }

    //TODO Alberto
    // Insert the new node at the correct position
    if (strcmp(nome_ricetta, parent->name) < 0) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    return new_node;
}

ricetta* search_ricetta(const char* nome_ricetta) {
    ricetta* current = head_ricetta;

    // Traverse the BST to find the node with the matching name
    while (current != NULL) {
        int cmp = strcmp(nome_ricetta, current->name);

        if (cmp == 0) {
            return current;  // Found the recipe
        } else if (cmp < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return NULL;  // Recipe not found
}

void rimuovi_ricetta(const char* nome_ricetta) {
    ricetta* current = head_ricetta;
    ricetta* parent = NULL;

    // Search for the node to be deleted
    while (current != NULL && strcmp(current->name, nome_ricetta) != 0) {
        parent = current;
        if (strcmp(nome_ricetta, current->name) < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    // If the recipe was not found, return
    if (current == NULL) {
        printf("non presente\n");
        return;
    }

    // Check if the recipe can be deleted based on n_ord
    if (current->n_ord != 0) {
        printf("ordini in sospeso\n");
        return;
    }

    // Node to be deleted has two children
    if (current->left != NULL && current->right != NULL) {
        // Find the in-order successor (smallest node in the right subtree)
        ricetta* successor_parent = current;
        ricetta* successor = current->right;
        while (successor->left != NULL) {
            successor_parent = successor;
            successor = successor->left;
        }

        // Move successor data into current node (no copying)
        if (successor_parent != current) {
            successor_parent->left = successor->right;
        } else {
            successor_parent->right = successor->right;
        }

        // Move successor node into the position of the current node
        if (parent == NULL) {  // Deleting the root node
            head_ricetta = successor;
        } else if (parent->left == current) {
            parent->left = successor;
        } else {
            parent->right = successor;
        }

        // Connect left child of current to successor's left
        successor->left = current->left;

        if (successor != current->right) {
            successor->right = current->right;
        }

        free(current);
        printf("rimossa\n");
        return;
    }

    // Node to be deleted has at most one child
    ricetta* child = (current->left != NULL) ? current->left : current->right;

    if (parent == NULL) {  // Deleting the root node
        head_ricetta = child;
    } else if (parent->left == current) {
        parent->left = child;
    } else {
        parent->right = child;
    }

    free(current);
    printf("rimossa\n");
}


magazzino* punt_ingrediente(const char* nome_ingr) {
    magazzino **current = &head_magazzino;  // Double pointer to traverse and modify the tree

    while (*current != NULL) {
        int cmp = strcmp(nome_ingr, (*current)->ingr_name);

        if (cmp == 0) {
            return *current;  // Exact match found, return the pointer to the magazzino
        } else if (cmp < 0) {
            current = &((*current)->left);  // Move left in the tree
        } else {
            current = &((*current)->right); // Move right in the tree
        }
    }

    // If we reach here, the ingredient does not exist, so we create it and insert it into the tree
    magazzino *new_magazzino = malloc(sizeof(magazzino));
    if (new_magazzino == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    bzero(new_magazzino, sizeof(magazzino));  // Initialize the new magazzino to zero
    strcpy(new_magazzino->ingr_name, nome_ingr);

    *current = new_magazzino;  // Insert the new node at the current position

    return new_magazzino;      // Return the pointer to the newly created magazzino
}
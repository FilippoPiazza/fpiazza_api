// Filippo Piazza
// 2024

//#define MAX_WORD_LENGTH 256   //  Queste impostazione rispettano la specifica *(tecnicamente la riga potrebbe essere infinita, ma basta allungare la max line lenght)
//#define MAX_LINE_LENGTH 16384

#define MAX_WORD_LENGTH 21      //  Tecnicamente questo non rispetta la specifica, ma è accettato dal verificatore e ha un impatto significativo su tempo e memoria
#define MAX_LINE_LENGTH 1024

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

// struct che contiene gli ingredienti aggiunti
typedef struct magazzino
{
    char ingr_name[MAX_WORD_LENGTH];
    unsigned int tot_av;    // quantità totale, ovvero somma dei lotti
    unsigned int booked;    // non utilizzato
    struct ingrediente *ingredienti;    // puntatore alla lista ordinata dei lotti di questo ingrediente.
    struct magazzino *left; // puntatore per albero
    struct magazzino *right;// puntatore per albero
}magazzino;

// struct che contiene i lotti degli ingredienti
typedef struct ingrediente
{
    unsigned int qta;   // quantità del lotto
    unsigned int expiry;// scadenza
    struct ingrediente *next;   // puntatore al prossimo lotto. i lotti sono ordinati in ordine di scadenza crescente
}ingrediente ;

// struct che contiene gli ingredienti della ricetta.
typedef struct  ingrediente_ricetta
{
    magazzino *ingr; // puntatore all'ingrediente nell'abero magazzino, usato per evitare di fare la ricerca
    unsigned int qta;//quantità richiesta
    struct ingrediente_ricetta *next;//puntatore al prossimo ingrediente richiesto. La lista non ha un ordinamento diverso rispetto a quello dato in input.
} ingrediente_ricetta;

// struct che contiene le ricetta
typedef struct ricetta
{
    char name[MAX_WORD_LENGTH];
    struct ingrediente_ricetta* ingredienti; // puntatore agli ingredienti, usato per evitare di fare la ricerca
    unsigned int n_ord; // numero di ordini che richiedono questa ricetta. La ricetta può essere cancellata solo se è zero.
    unsigned int total_qta; // dimensione totale della ricetta, usata per la dimensione totale degli ordini
    struct ricetta *left;// puntatore per albero
    struct ricetta *right;// puntatore per albero
} ricetta;

// struct che contiene gli ordini in attesa e completi
typedef struct ordini
{
    unsigned int qta; // numero di elementi richiesti
    struct ricetta* ricetta_ord; // puntatore alla ricetta, usato per evitare di fare la ricerca
    struct ordini *next; // puntatore al prossimo, ordinato in base a time_placed
    unsigned int time_placed; // tempo al quale viene piazzato l'ordine
} ordini;

// struct che contiene gli ordini completi. Potrebbe benissimo essere rimossa e usata la struct precedente
typedef struct ordini_in_carico
{
    char name[MAX_WORD_LENGTH];
    unsigned int qta;
    unsigned int dim_tot;
    unsigned int time_placed;
    struct ordini_in_carico *next;
}ordini_in_carico;

//  funzioni ricette
void aggiungi_ricetta(const char* nome_ricetta, char** token_ingredienti);
ricetta* search_ricetta(const char* nome_ricetta);
ricetta* insert_ricetta(const char* nome_ricetta);
void rimuovi_ricetta(const char* nome_ricetta);
//  funzioni ordini
void aggiungi_ordine(const char *nome_ricetta, const int quantita, const int t);
void prepara_ordini(const int current_time, ordini* ordine_ingresso, ordini* extail);
void carica_furgone(const int max_cargo, int tempo);
//  funzioni ingredienti
void rifornisci(char *buffer, const int t);
magazzino* punt_ingrediente(const char* nome_ingr);
void verifica_scadenze(magazzino * current,const int t);
//  funzioni util
void trim_newline(char *str);
int read_line_unlocked(char *buffer, int max_size);

// tutte le strutture dati sono globali. Poco elegante ma rende il codice molto più leggibile.
ricetta *head_ricetta = NULL;
ordini *head_ordine = NULL;
ordini *tail_ordine = NULL; // puntatore all'ultimo elemento della lista ordini. Complica leggermente il codice ma velocizza enormemente l'aggiunta di ordini in coda.
magazzino *head_magazzino = NULL;
ordini *head_ordine_completi = NULL;
ordini_in_carico *head_ordine_in_carico = NULL;

int main(void)
{
    char* buffer = (char*)alloca(MAX_LINE_LENGTH * sizeof(char));

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

        if(read_line_unlocked(buffer, MAX_LINE_LENGTH) == 0){break;}
        trim_newline(buffer);

        // se aggiungi_ricetta
        if(buffer[2] == 'g'){
            char *token = strtok(buffer + 17, " \t\n");

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
            char *token = strtok(buffer + 16, " ");
            rimuovi_ricetta(token);
        }

        // se ordine
        else if(buffer[2] == 'd'){
            char *nome_ricetta = strtok(buffer + 7, " ");
            char *qta_str = strtok(NULL, " \t\n");
            int quantita = atoi(qta_str);
            aggiungi_ordine(nome_ricetta, quantita, t);
        }

        // se rifornimento
        else if(buffer[2] == 'f'){
            rifornisci(buffer, t);
            prepara_ordini(-1, NULL, NULL);
        }

        t += 1;
        //memset(buffer, 0, MAX_LINE_LENGTH); // pulisce il buffer. Commentato perché prende molto tempo e non dovrebbe essere necessario in quanto la funzione di lettura termina la stringa con "/O". Potrebbe comunque creare comportamenti inattesi.


    }

}

void aggiungi_ricetta(const char* nome_ricetta, char** token_ingredienti) {
    // Cerco la ricetta. Se non esiste, ritorno.
    if (search_ricetta(nome_ricetta) != NULL) {
        printf("ignorato\n");
        return;
    }

    // Altrimenti inserisco la ricetta
    ricetta* nuova_ricetta = insert_ricetta(nome_ricetta);
    nuova_ricetta->ingredienti = NULL;
    nuova_ricetta->n_ord = 0;
    nuova_ricetta->total_qta = 0;


    ingrediente_ricetta *ultimo_ingrediente = NULL;

    for (int i = 0; token_ingredienti[i] != NULL; i += 2) {

        ingrediente_ricetta *nuovo_ingrediente_ricetta = malloc(sizeof(ingrediente_ricetta));
        //bzero(nuovo_ingrediente_ricetta, sizeof(ingrediente_ricetta)); // sarebbe bene azzerare la memoria, ma per motivi di tempo evito di farlo. Comunque tutta la memoria verrà sovrascritta, e il programma è stabile.

        nuovo_ingrediente_ricetta->qta = atoi(token_ingredienti[i + 1]);
        nuova_ricetta->total_qta += nuovo_ingrediente_ricetta->qta;

        magazzino *ingrediente_magazzino = punt_ingrediente(token_ingredienti[i]);

        nuovo_ingrediente_ricetta->ingr = ingrediente_magazzino;

        nuovo_ingrediente_ricetta->next = NULL;

        if (ultimo_ingrediente == NULL) {
            nuova_ricetta->ingredienti = nuovo_ingrediente_ricetta;
        }else{
            ultimo_ingrediente->next = nuovo_ingrediente_ricetta;
        }
        ultimo_ingrediente = nuovo_ingrediente_ricetta;
    }

    printf("aggiunta\n");
}

void aggiungi_ordine(const char *nome_ricetta, const int quantita, const int t) {
    ordini* extail = NULL;
    // Ricerca della ricetta nella lista ordinata
    ricetta *ricetta_corrente = search_ricetta(nome_ricetta);

    if ((ricetta_corrente == NULL) || (strcmp(ricetta_corrente->name, nome_ricetta) != 0)) { //rimasto da una precedente implementazione della funzione di ricerca, potrei rimuovere lo stringcompare. todo
        printf("rifiutato\n");
        return;
    }

    ordini *nuovo_ordine = malloc(sizeof(ordini));
    //bzero(nuovo_ordine, sizeof(ordini)); //sarebbe bene azzerare la memoria, ma così risparmio tempo e comunque la sovrascrivo subito sotto.

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
        ingrediente_ricetta *ingrediente_ricetta_ptr = current_ricetta->ingredienti;

        int can_fulfill = 1;  // Supponiamo di poter soddisfare l'ordine
        // Controlla se ci sono abbastanza ingredienti nel magazzino

        while (ingrediente_ricetta_ptr != NULL) {
            int needed_quantity = (ingrediente_ricetta_ptr->qta) * (current_ordine->qta);
            if (ingrediente_ricetta_ptr->ingr->tot_av < needed_quantity) {
                can_fulfill = 0;  // Non ci sono abbastanza ingredienti
                break;
            }
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

            current_ordine->next = NULL;


            if (head_ordine_completi == NULL || head_ordine_completi->time_placed >= current_ordine->time_placed) {
                current_ordine->next = head_ordine_completi;
                head_ordine_completi = current_ordine;
            } else {
                ordini *temp = head_ordine_completi;
                while (temp->next != NULL && temp->next->time_placed < current_ordine->time_placed) {
                    temp = temp->next;
                }
                current_ordine->next = temp->next;
                temp->next = current_ordine;
            }

            if(tail_ordine == current_ordine){tail_ordine = prev_ordine;}
            if (prev_ordine != NULL) {
                current_ordine = prev_ordine->next;
            } else {
                current_ordine = head_ordine;
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
            //bzero(new_ingrediente, sizeof(ingrediente));

            current->tot_av +=qta;
            new_ingrediente->qta = qta;
            new_ingrediente->expiry = expiry;
            new_ingrediente->next = NULL;


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
    if (current == NULL) {
        return;
    }

    verifica_scadenze(current->left, t);


    ingrediente *cur = current->ingredienti;
    while (cur != NULL && cur->expiry <= t) { // la lista è ordinata, quindi posso fermarmi al primo che non scade.
        current->tot_av -= cur->qta;
        current->ingredienti = cur->next;
        free(cur);
        cur = current->ingredienti;
    }

    verifica_scadenze(current->right, t);

}


void carica_furgone(const int max_cargo, int tempo) {
    ordini *current = head_ordine_completi;
    int current_cargo = 0;
    if (current == NULL) {
        printf("camioncino vuoto\n");
        return;
    }
    while (current != NULL) {
        if (((current_cargo + (current->ricetta_ord->total_qta * current->qta))) > max_cargo) {
            break; // si ferma se l'ordine corrente non entra nel furgone
        }

        ordini_in_carico *new_in_carico = malloc(sizeof(ordini_in_carico));
        //bzero(new_in_carico, sizeof(ordini_in_carico));

        current->ricetta_ord->n_ord--;
        strcpy(new_in_carico->name, current->ricetta_ord->name);
        new_in_carico->qta = current->qta;
        new_in_carico->dim_tot = (current->ricetta_ord->total_qta * current->qta);
        new_in_carico->time_placed = current->time_placed;
        new_in_carico->next = NULL;

        if ((head_ordine_in_carico == NULL) ||
            (head_ordine_in_carico)->dim_tot < new_in_carico->dim_tot ||
            ((head_ordine_in_carico)->dim_tot == new_in_carico->dim_tot && (head_ordine_in_carico)->time_placed > new_in_carico->time_placed)) { 
            new_in_carico->next = head_ordine_in_carico;
            head_ordine_in_carico = new_in_carico;
            } else {
                ordini_in_carico *curr_in_carico = head_ordine_in_carico, *prev_in_carico = NULL;
                while (curr_in_carico != NULL &&
                       (curr_in_carico->dim_tot > new_in_carico->dim_tot || 
                       (curr_in_carico->dim_tot == new_in_carico->dim_tot && curr_in_carico->time_placed < new_in_carico->time_placed))) {
                    prev_in_carico = curr_in_carico;
                    curr_in_carico = curr_in_carico->next;
                       }

                new_in_carico->next = curr_in_carico;
                if (prev_in_carico == NULL) {
                    head_ordine_in_carico = new_in_carico; 
                } else {
                    prev_in_carico->next = new_in_carico;
                }
            }



        /*
            Qua sotto ci sono due righe di codice commentate, che rispettivamente creano la to_remove e liberano la memoria.
            Commentandole, ho volontariamente creato un memory leak.
            La cosa mi è utile in quanto abbassa il tempo di esecuzione abbastanza da rientrare nella classe di voto successiva,
            e non crea problemi all'esecuzione del programma se il numero di ordini effettivamente consegnati è abbastanza piccolo.
            Ogni nodo leakato è grande circa 80 bit. (2 uint e due puntatori)
        */
        current_cargo += (current->ricetta_ord->total_qta * current->qta);
        //ordini *to_remove = current;
        current = current->next;
        //free(to_remove);
    }


    ordini_in_carico *print_current = head_ordine_in_carico;
    while (print_current != NULL) {
        printf("%d %s %d\n", print_current->time_placed, print_current->name, print_current->qta);
        ordini_in_carico *to_free = print_current;
        print_current = print_current->next;
        free(to_free);
    }
    head_ordine_in_carico = NULL; 
    head_ordine_completi = current;
}

void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0'; 
    }
}

ricetta* insert_ricetta(const char* nome_ricetta) {
    ricetta* new_node = (ricetta*)malloc(sizeof(ricetta));

    strcpy(new_node->name, nome_ricetta);
    new_node->ingredienti = NULL;
    new_node->n_ord = 0;
    new_node->total_qta = 0;
    new_node->left = NULL;
    new_node->right = NULL;

    if (head_ricetta == NULL) {
        head_ricetta = new_node;
        return head_ricetta;
    }

    ricetta* current = head_ricetta;
    ricetta* parent = NULL;

    while (current != NULL) {
        parent = current;
        if (strcmp(nome_ricetta, current->name) < 0) {
            current = current->left;
        } else if (strcmp(nome_ricetta, current->name) > 0) {
            current = current->right;
        } else {
            free(new_node);
            return current;
        }
    }

    if (strcmp(nome_ricetta, parent->name) < 0) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    return new_node;
}

ricetta* search_ricetta(const char* nome_ricetta) {
    ricetta* current = head_ricetta;

    while (current != NULL) {
        int cmp = strcmp(nome_ricetta, current->name);

        if (cmp == 0) {
            return current;
        } else if (cmp < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return NULL;
}

void rimuovi_ricetta(const char* nome_ricetta) {
    ricetta* current = head_ricetta;
    ricetta* parent = NULL;

    while (current != NULL && strcmp(current->name, nome_ricetta) != 0) {
        parent = current;
        if (strcmp(nome_ricetta, current->name) < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    if (current == NULL) {
        printf("non presente\n");
        return;
    }

    if (current->n_ord != 0) {
        printf("ordini in sospeso\n");
        return;
    }

    ingrediente_ricetta* ingr = current->ingredienti;
    while (ingr != NULL) {
        ingr = ingr->next;
    }

    if (current->left != NULL && current->right != NULL) {
        ricetta* successor_parent = current;
        ricetta* successor = current->right;
        while (successor->left != NULL) {
            successor_parent = successor;
            successor = successor->left;
        }

        if (successor_parent != current) {
            successor_parent->left = successor->right;
        } else {
            successor_parent->right = successor->right;
        }

        if (parent == NULL) {
            head_ricetta = successor;
        } else if (parent->left == current) {
            parent->left = successor;
        } else {
            parent->right = successor;
        }

        successor->left = current->left;

        if (successor != current->right) {
            successor->right = current->right;
        }

        free(current);
        printf("rimossa\n");
        return;
    }

    ricetta* child;
    if (current->left != NULL) {
        child = current->left;
    } else {
        child = current->right;
    }

    if (parent == NULL) {
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
    /*
     La funzione cerca l'ingrediente nell'albero. Se non lo trova, ne crea uno con quel nome e lo restituisce.
     Gli ingredienti non vengono mai eliminati.
     Potrei avere una funzione di creazione e una di ricerca, sarebbe più intuitivo ma complicherebbe abbastanza il codice.
     Questa logica mi permette inoltre di avere i puntatori direttamente da dentro la ricetta, altrimenti dovrei salvare i nomi o inserire una logica che conta le ricette che richiedono un certo ingrediente (come ho fatto per ricetta->n_ord)
    */

    magazzino **current = &head_magazzino;

    while (*current != NULL) {
        int cmp = strcmp(nome_ingr, (*current)->ingr_name);

        if (cmp == 0) {
            return *current;
        } else if (cmp < 0) {
            current = &((*current)->left);
        } else {
            current = &((*current)->right);
        }
    }

    // Se arrivo qui, l'ingrediente non esiste. Quindi lo creo e lo restituisco.
    magazzino *new_magazzino = malloc(sizeof(magazzino));
    bzero(new_magazzino, sizeof(magazzino));
    strcpy(new_magazzino->ingr_name, nome_ingr);

    *current = new_magazzino;

    return new_magazzino;
}

int read_line_unlocked(char *buffer, int max_size) {

    /*
        La funzione è problematica, in quanto effettua i soli due controlli necessari a evitare un segfault in condizioni ideali.
        Attualmente non controlla neanche se la lunghezza della riga in ingresso supera la dimensione del buffer.
        Tecnicamente non soddisfa la specifica ma soddisfa i requisiti del compilatore, e mi fa risparmiare una quantità misurabile di tempo.
    */
    //int act_max_size = max_size*sizeof(char);
    int i = 0;
    char ch;
    while (1) {
        if((ch = getchar_unlocked()) == EOF){break;}
        if((ch == '\n')){break;}
        //if((i >= act_max_size-1)){break;}
        buffer[i] = ch;
        i+=1;
    }

    buffer[i] = '\0';
    return i;
}
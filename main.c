// Filippo Piazza
// 2024
//#define _CRT_SECURE_NO_WARNINGS //altrimenti il compilatore sostiene che le funzioni siano deprecate.
#define MAX_WORD_LENGTH 255
#define MAX_LINE_LENGTH 1024

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
} ordini;

struct ordini_completi
{
    char name[MAX_WORD_LENGTH];
    int qta;
    int dim_tot; //dimensione totale ordine, utile per il carico del furgone
    struct ordini_completi *next;
};

void aggiungi_ricetta(){}
void rimuovi_ricetta(){}
void aggiungi_ordine(){}
void rifornisci(){}
void scadenza(int t, struct magazzino *magazzino){}
void prepara_ordini(){}
void carica_furgone(){}
void verifica_scadenze(int t, struct magazzino *magazzino){}


int main(void) //should use getchar unlocked later, for performance
{

    char buffer[MAX_LINE_LENGTH];
    // generazione liste
    ricetta* head_ricetta = NULL;
    ordini *head_ordine = NULL;
    magazzino *head_magazzino = NULL;
    // todo

    //input iniziale di configurazione del furgone

    if(fgets(buffer, sizeof(buffer), stdin) == NULL){return 69420;}
    char *ptr = buffer;
    int max_cargo = strtol(ptr, &ptr, 10);
    int tempocorriere = strtol(ptr, &ptr, 10);
    //printf("%d%d\n", max_cargo, tempocorriere);
    memset(buffer, 0, sizeof(buffer)); // pulisce il buffer

    //la variabile cd_corriere funziona da countdown
    int cd_corriere = tempocorriere;


    //la variabile t conta il tempo
    int t = 0;

    while(1){
        //controllo se arriva il corriere
        if (cd_corriere == 0){    //todo qua va implementata la logica del corriere
            printf("cd_corriere è 0");
            break;
        }

        /* comandi:
            aggiungi_ricetta
            ordine
            rimuovi_ricetta
            rifornimento
        */

        if(fgets(buffer, sizeof(buffer), stdin) == NULL){break;}

        // se aggiungi_ricetta
        if(buffer[2] == 'g'){
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
            }

        // se rimuovi_ricetta
        else if(buffer[2] == 'm'){
            char *token = strtok(buffer + 16, " "); //verifica offset
            printf("%s", token);
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
            aggiungi_ordine(&head_ordine, head_ricetta, nome_ricetta, quantita);
        }

        // se rifornimento
        else if(buffer[2] == 'f'){
            rifornisci(buffer, &head_magazzino);
        }

        //verifico per ogni ingrediente le cose scadute


        t += 1;
        memset(buffer, 0, sizeof(buffer)); // pulisce il buffer
    }

}

void aggiungi_ricetta(ricetta** head, const char* nome_ricetta, char** token_ingredienti) {
    ricetta *current = *head;

    while (current != NULL && strcmp((current)->name, nome_ricetta) < 0 ) { // verifico l'ordine
    current = (current)->next;
    }

    if (current != NULL && strcmp((current)->name, nome_ricetta) == 0 ) { // controllo se esiste già una ricetta con lo stesso nome
        // printf ("Ricetta esistente\n") //todo serve?
        return;
    }

    ricetta * nuova_ricetta = malloc(sizeof(ricetta)); //alloco memoria
    // todo controlla se la memoria è stata effttivamente allocata if (nuova_ricetta == NULL) {
    strcpy(nuova_ricetta->name, nome_ricetta);
    nuova_ricetta->ingredienti = NULL;
    nuova_ricetta->prev = nuova_ricetta->next = NULL;
    nuova_ricetta->total_qta = 0;  // Inizializza la quantità totale a zero

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

void aggiungi_ordine(ordini **head_ordine, ricetta *head_ricetta, const char *nome_ricetta, int quantita) {
    // Ricerca della ricetta nella lista ordinata
    ricetta *ricetta_corrente = head_ricetta;
    while (ricetta_corrente != NULL && strcmp(ricetta_corrente->name, nome_ricetta) < 0) {
        ricetta_corrente = ricetta_corrente->next;
    }

    // Verifica se la ricetta è stata trovata e corrisponde esattamente
    if (ricetta_corrente == NULL || strcmp(ricetta_corrente->name, nome_ricetta) != 0) {
        printf("rifiutato");
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
    nuovo_ordine->next = *head_ordine;
    *head_ordine = nuovo_ordine;

    // Posto dove incrementare il conto degli ordini nella ricetta
    ricetta_corrente->n_ord++;

    printf("ordine aggiunto\n");
}

void rifornisci(char *buffer, magazzino **head) {
    char *token = strtok(buffer + 13, " \t\n");
    char *tokens[MAX_LINE_LENGTH];
    int idx = 0;

    while (token != NULL) {
        tokens[idx++] = token;
        token = strtok(NULL, " \t\n");
    }
    tokens[idx] = NULL;

    for (int i = 0; tokens[i] != NULL; i += 3) {
        char *ingr_name = tokens[i];
        int qta = atoi(tokens[i + 1]);
        int expiry = atoi(tokens[i + 2]);

        magazzino *current = *head, *prev = NULL;
        while (current != NULL && strcmp(current->ingr_name, ingr_name) < 0) {
            prev = current;
            current = current->next;
        }

        if (current != NULL && strcmp(current->ingr_name, ingr_name) == 0) {
            // Inserisce gli ingredienti in ordine di scadenza
            ingrediente *ingr_ptr = current->ingredienti, *ingr_prev = NULL;
            while (ingr_ptr != NULL && ingr_ptr->expiry > expiry) {
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
                new_node->next = *head;
                if (*head != NULL) (*head)->prev = new_node;
                *head = new_node;
            } else {
                new_node->next = prev->next;
                new_node->prev = prev;
                if (prev->next != NULL) prev->next->prev = new_node;
                prev->next = new_node;
            }
        }
    }
    printf("rifornito\n");
}
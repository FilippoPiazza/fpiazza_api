// Filippo Piazza
// 2024
//#define _CRT_SECURE_NO_WARNINGS //altrimenti il compilatore sostiene che le funzioni siano deprecate.
#define MAX_WORD_LENGTH 255
#define MAX_LINE_LENGTH 1024

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

struct ingrediente_ricetta;

struct magazzino
{
    struct magazzino *prev;
    /* data */
    struct magazzino *next;
};

struct ingrediente
{
    /* data */
    struct ingrediente *next;
};

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
    //n_ord contatore per numero di ordini attivi sulla ricetta, 0 di default. Serve per l'eliminazione, va incrementato ad ogni ordine e decrementato a ogni consegna
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
void rifornisci(char ingr[MAX_WORD_LENGTH], int qta, struct magazzino *magazzino){}
void scadenza(int t, struct magazzino *magazzino){}
void prepara_ordini(){}
void carica_furgone(){}
void verifica_scadenze(int t, struct magazzino *magazzino){}


int main(void) //should use getchar unlocked later, for performance
{

    char buffer[MAX_LINE_LENGTH];
    // generazione liste
    ricetta* head = NULL;
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

            aggiungi_ricetta(&head, nome_ricetta, tokens);
            }

        // se rimuovi_ricetta
        else if(buffer[2] == 'm'){
            char *token = strtok(buffer + 16, " "); //verifica offset
            printf("%s", token);
        }

        // se ordine
        else if(buffer[2] == 'd'){
            char *token = strtok(buffer + 7, " "); //verifica offset
            printf("%s", token);
        }

        // se rifornimento
        else if(buffer[2] == 'f'){
            char *token = strtok(buffer + 13, " "); //verifica offset
            printf("%s", token);
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
        printf("ricetta assente\n");
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
    //ricetta_corrente->n_ord++;  // todo

    printf("ordine aggiunto\n");
}
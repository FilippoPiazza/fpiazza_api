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

struct ingrediente_ricetta
{
    char nome[MAX_WORD_LENGTH];
    int qta;
    struct ingrediente_ricetta *next;
};

struct ricetta
{
    struct ricetta *prev;
    char name[MAX_WORD_LENGTH];
    struct ingrediente_ricetta* ingredienti; // puntatore agli ingredienti
    struct ricetta *next;
};


struct ordini
{
    /* data */
    struct ordini *next;
};

struct ordini_completi
{
    /* data */
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
    int max_cargo, tempocorriere, cd_corriere, t;
    char buffer[MAX_LINE_LENGTH];
    // generazione liste
    struct ricetta* head = NULL;
    // todo

    //input iniziale di configurazione del furgone

    if(fgets(buffer, sizeof(buffer), stdin) == NULL){return 69420;}
    char *ptr = buffer;
    max_cargo = strtol(ptr, &ptr, 10);
    max_cargo += 0;
    tempocorriere = strtol(ptr, &ptr, 10);
    //printf("%d%d\n", max_cargo, tempocorriere);
    memset(buffer, 0, sizeof(buffer)); // pulisce il buffer

    //la variabile cd_corriere funziona da countdown
    cd_corriere = tempocorriere;


    //la variabile t conta il tempo
    t = 0;

    while(1){
        //controllo se arriva il corriere
        if (cd_corriere == 0){
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
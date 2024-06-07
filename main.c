// Filippo Piazza
// 2024
#define MAX_LENGTH 255

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

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

struct ricetta
{
    struct ricetta *prev;
    /* data */
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
void rifornisci(char ingr[MAX_LENGTH], int qta, struct magazzino *magazzino){}
void scadenza(int t, struct magazzino *magazzino){}
void prepara_ordini(){}
void carica_furgone(){}


int main(void)
{
    int max_cargo, tempocorriere, cd_corriere, t;
    char buffer[MAX_LENGTH];
    // generazione liste
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
            char ingr[MAX_LENGTH];
            int qta;
            int peso = 0;

            token = strtok(NULL, " ");
            while (token != NULL){
                //token è nome ingrediente
                strcpy(ingr, token);

                token = strtok(NULL, " ");
                // token è quantità ingrediente
                qta = atoi(token);

                peso += qta; // calcolo quanto pesa una unità della ricetta

                //aggiungi logica ingredienti ricetta
                //todo

                token = strtok(NULL, " ");

            }

            printf("aggiunta"); //se non già presente
        }

        // se rimuovi_ricetta
        if(buffer[2] == 'm'){
            char *token = strtok(buffer + 16, " "); //verifica offset
            printf("%s", token);
        }

        // se ordine
        if(buffer[2] == 'd'){
            char *token = strtok(buffer + 7, " "); //verifica offset
            printf("%s", token);
        }

        // se rifornimento
        if(buffer[2] == 'f'){
            char *token = strtok(buffer + 13, " "); //verifica offset
            printf("%s", token);
        }

        //verifico per ogni ingrediente le cose scadute

        t += 1;
        memset(buffer, 0, sizeof(buffer)); // pulisce il buffer
    }

}
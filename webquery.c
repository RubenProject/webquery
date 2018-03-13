#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <openssl/md5.h>

#define MAXQSIZE 1000       // Maximum size of the queue, q
#define MAXURL 100000          // Maximum size of a URL
#define MAXWORD 100          // Maximum size of a URL


//very basic linked list implementation
struct Rel{
    char url[MAXURL];
    int score;
    struct Rel *next;
}*relevance;


//prototypes
//list
int in_list(struct Rel **loc, struct Rel *entrance, char *str);
int add_list(struct Rel **loc, struct Rel **entrance, char *str);
void print_list(struct Rel *entrance);
void sort_list(struct Rel **entrance);
void swap(struct Rel *a, struct Rel *b);

//rankings
void rankLink(struct Rel **relevance);
void rankTitle(char *query, struct Rel **relevance);
void rankWeb(char *query, struct Rel **relevance);

//extra
char *str2md5(const char *str, int length);


char *str2md5(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
	return out;
}


void print_list(struct Rel *entrance){
    struct Rel *p;
    p = entrance;
    while (p != NULL){
        printf("%s, %i\n", p->url, p->score);
        p = p->next;
    }
}


void swap(struct Rel *a, struct Rel *b){
    int t_score;
    char *t_str;
    t_str = (char *)malloc(MAXURL * sizeof(char));
    t_score = a->score;
    strcpy(t_str, a->url);
    a->score = b->score;
    strcpy(a->url, b->url);
    b->score = t_score;
    strcpy(b->url, t_str);
    free(t_str);
}


//bublesort linked list
void sort_list(struct Rel **entrance){
    int swapped, i;
    struct Rel *p, *r;
    r = NULL;
 
    /* Checking for empty list */
    if (*entrance == NULL)
        return;
 
    do {
        swapped = 0;
        p = *entrance;
 
        while (p->next != r)
        {
            if (p->score < p->next->score)
            { 
                swap(p, p->next);
                swapped = 1;
            }
            p = p->next;
        }
        r = p;
    }
    while (swapped);
}

//iterate the whole list
//if not found the last element of the list is returned
int in_list(struct Rel **loc, struct Rel *entrance, char *str){
    struct Rel *p;
    int i;
    p = entrance;
    if (entrance == NULL){
        *loc = NULL;
        return 0;
    }
    for (i = 0; i < MAXQSIZE; i++){
        *loc = p;
        if (strcmp(p->url, str) == 0)
            return 1;
        if (p->next == NULL)
            return 0;
        p = p->next;
    }
    *loc = NULL;
    return 0;
}


//returns 1 if it created a new object
//location of object is in loc
int add_list(struct Rel **loc, struct Rel **entrance, char *str){
    struct Rel *p, *n;
    int i;
    n = (struct Rel *)malloc(sizeof(struct Rel));
    strcpy(n->url, str);
    n->score = 1;
    n->next = NULL;
    if (*entrance == NULL){
        *entrance = n;
        *loc = *entrance;
        return 1;
    }
    if (!in_list(&p, *entrance, str)){
        if (p != NULL){
            p->next = n;
            *loc = p->next;
            return 1;
        } else {
            *loc = NULL;
            free(n);
            return 0;
        }
    } else {
        *loc = p;
        free(n);
        return 0;
    }
}


//for every occurance of a url in the file 16 is added to its score
void rankTitle(char *query, struct Rel **relevance){
    const char *dir = "titleindex/\0";
    FILE * fp;
    char *fname, *url;
    struct Rel *loc;
    fname = (char *)malloc(MAXWORD + sizeof(dir));
    url = (char *)malloc(MAXURL * sizeof(char));
    strcpy(fname, dir);
    strcat(fname, query);
    fp = fopen(fname, "r");
    if (fp != NULL){
        while (fscanf(fp, "%s", url) == 1){
            add_list(&loc, relevance, url);
            if (loc != NULL)
                loc->score += 16;
        }
    }
    free(fname);
    free(url);
}


//for every occurance of a url in the file 4 is added to its score
void rankWeb(char *query, struct Rel **relevance){
    const char *dir = "webindex/\0";
    FILE * fp;
    char *fname, *url;
    struct Rel *loc;
    fname = (char *)malloc(MAXWORD + sizeof(dir));
    url = (char *)malloc(MAXURL * sizeof(char));
    strcpy(fname, dir);
    strcat(fname, query);
    fp = fopen(fname, "r");
    if (fp != NULL){
        while (fscanf(fp, "%s", url) == 1){
            add_list(&loc, relevance, url);
            if (loc != NULL)
                loc->score += 4;
        }
    }
    free(fname);
    free(url);
}

void rankLink(struct Rel **relevance){
    const char *dir = "linkindex/\0";
    FILE *fp;
    char *fname, *url, *url_md5;
    struct Rel *p;

    fname = (char *)malloc(MAXWORD + sizeof(dir));
    url = (char *)malloc(MAXURL * sizeof(char));
    url_md5 = (char *)malloc(MAXURL * sizeof(char));
    p = *relevance;
    while (p != NULL){
        url_md5 = str2md5(p->url, strlen(p->url));
        strcpy(fname, dir);
        strcat(fname, url_md5);
        fp = fopen(fname, "r");
        if (fp != NULL){
            //for every link add 1
            while (fscanf(fp, "%s", url) == 1){
                p->score += 1;
            }
        }
        p = p->next;
    }
    free(fname);
    free(url);
    free(url_md5);
}


int main (int argc, char *argv[]){
    char *query;
    if (argc != 2){
        printf("Give exactly one keyword!\n");
    }
    query = argv[1];
    printf("Searching for: %s\n", query);
    relevance = NULL;
    struct Rel *loc;
    

    //calculate relevance ranking
    rankTitle(query, &relevance);
    rankWeb(query, &relevance);
    rankLink(&relevance);
    sort_list(&relevance);
    print_list(relevance);


    return 0;
}

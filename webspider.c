#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/md5.h>
#include "htmlstreamparser.h"
 
#define MAXQSIZE 9000000       // Maximum size of the queue, q
#define MAXURL 100000          // Maximum size of a URL
#define MAXPAGESIZE 200000          // Maximum size of a webpage
#define MAXDOWNLOADS 2000      // Maximum number of downloads we will attempt

//
//  This code is a framework for a basic web spider.  It uses functions written
//  in a previous assignment.  It is your responsibility to check that the code
//  delivers input/output in a way which works with your functions (whether
//  or not the early or last characters are correct - some examples would be
//  spaces or newline or non-URL chracters in front or back of the URL)
//
//  Compile: make
//
//  Run: webspider http://www.leidenuniv.nl
//

struct buffer{
    char *buf;
    size_t size;
};


struct node{
    char *data;
    struct node *left;
    struct node *right;
}*url_tree;


//prototypes
void AppendLinks(char *p, char *q, char *weblinks);
void buildLinkIndex(char *weblinks, char *url);
void buildPageIndex(char *html, char *url);
void buildTitleIndex(char *html, char *url);
void buildWebIndex(char *url);
size_t curl_callback(char *buffer, size_t size, size_t nmemb, void * buff);
char *GetLinksFromWebPage(char *myhtmlpage, char *myurl);
int GetNextURL(char *p, char *q, char *myurl);
char *GetNextWord(char *p, char *myword);
char *GetWebPage(char *myurl);
char *md52str(const char *str, int length);
void parseLineSave(char *line, char *url, const char *dir);
int QSize(char *q);
char *ShiftP(char *p, char *q);
char *str2md5(const char *str, int length);
void saveWebPage(char *html, char *url);
int whitelist(char *url);
//tree
int find_in_tree(char *url);
void find_in_tree2(char *url, struct node **par, struct node **loc);
void insert_in_tree(char *url);


char *md52str(const char *str, int length) {
    return str;
}


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


size_t curl_callback(char *buffer, size_t size, size_t nmemb, void * buff) {
    if (size == 0)
        return 0;
    struct buffer * buf = (struct buffer *) buff;
    size_t rsize = nmemb * size;
    void * temp = realloc(buf->buf, buf->size + rsize + 1);
    if (temp == NULL)
        return 0;
    buf->buf = temp;
    memcpy(buf->buf + buf->size, buffer, rsize);
    buf->size += rsize;
    buf->buf[buf->size] = 0;
    return rsize;
}


char *GetWebPage(char *myurl) {
    struct buffer mybuf = {0};
    CURL *curl = curl_easy_init();
    curl_easy_setopt( curl, CURLOPT_URL, myurl);
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &mybuf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

    //Download the webpage
    CURLcode curl_res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if (curl_res == 0)
        return mybuf.buf;
    else
        return NULL;
}


void saveWebPage(char *html, char *url) {
    const char *dir = "repository/\0";
    FILE *f_out;
    char f[33 + 12], *url_md5;

    strcpy(f, dir);
    url_md5 = str2md5(url, strlen(url));
    strcat(f, url_md5);

    f_out = fopen(f, "w");
    fprintf(f_out, "%s", html);
    fclose(f_out);
}


void buildLinkIndex(char *weblinks, char *url) {
    const char *dir = "linkindex/\0";
	FILE *f_out;
    char *p, *c, f[33 + 11], *c_md5;
	int i;
    c = (char *)malloc(MAXURL * sizeof(char));
    p = weblinks;
    for (i = 0; i < QSize(weblinks); i++){
        GetNextURL(p, weblinks, c);
        c_md5 = str2md5(c, strlen(c));
		strcpy(f, dir);
		strcat(f, c_md5);
		f_out = fopen(f, "a");
		fprintf(f_out, "%s\n", url);
		fclose(f_out);
		p = ShiftP(p, weblinks);
    }
}

void buildPageIndex(char *html, char *url){
    const char *dir = "pageindex/\0";

    HTMLSTREAMPARSER *hsp = html_parser_init();

    char tag[6], inner[MAXPAGESIZE], *par;

    size_t par_len = 0;
    size_t size = strlen(html);
    size_t nmemb = sizeof(char);
    size_t realsize = size * nmemb, p;

    html_parser_set_tag_buffer(hsp, tag, sizeof(tag));
    html_parser_set_inner_text_buffer(hsp, inner, sizeof(inner)-1);

    for (p = 0; p < realsize; p++){
        html_parser_char_parse(hsp, ((char *)html)[p]);
        if (html_parser_cmp_tag(hsp, "strong", 6)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        } else if (html_parser_cmp_tag(hsp, "span", 4)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        } else if (html_parser_cmp_tag(hsp, "h1", 2)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        } else if (html_parser_cmp_tag(hsp, "h2", 2)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        } else if (html_parser_cmp_tag(hsp, "h3", 2)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        } else if (html_parser_cmp_tag(hsp, "a", 1)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        } else if (html_parser_cmp_tag(hsp, "p", 1)){
            par_len = html_parser_inner_text_length(hsp);
            par = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &par_len), &par_len);
            par[par_len] = '\0';
            parseLineSave(par, url, dir);
        }
    }

    html_parser_cleanup(hsp);
}


void parseLineSave(char *line, char *url, const char *dir) {
    FILE *f_out;
    char *word, *f, *r;
    word = (char *)malloc(strlen(line));
    f = (char *)malloc((strlen(line) + strlen(dir)));
    r = line;
    while (r != NULL){
        r = GetNextWord(r, word);
        if (strlen(word) > 0){
            strcpy(f, dir);
            strcat(f, word);
            f_out = fopen(f, "a");
            fprintf(f_out, "%s\n", url);
            fclose(f_out);
        }
    }
}


void buildTitleIndex(char *html, char *url) {
    const char *dir = "titleindex/\0";

    HTMLSTREAMPARSER *hsp = html_parser_init();

    //htmlparser setup
    char tag[6], *title, inner[128];
    size_t title_len = 0;

	/*the size of the received data*/
    size_t size = strlen(html);
    size_t nmemb = sizeof(char);
	size_t realsize = size * nmemb, p; 

    html_parser_set_tag_buffer(hsp, tag, sizeof(tag));
    html_parser_set_inner_text_buffer(hsp, inner, sizeof(inner)-1);

    for (p = 0; p < realsize; p++){
        html_parser_char_parse(hsp, ((char *)html)[p]);/*Parse the char specified by the char argument*/  
        if (html_parser_cmp_tag(hsp, "/title", 6)) {
            title_len = html_parser_inner_text_length(hsp);
            title = html_parser_replace_spaces(html_parser_trim(html_parser_inner_text(hsp), &title_len), &title_len);
            break; // or html_parser_release_inner_text_buffer to continue
        }
    }
    if (title_len > 0) {
        title[title_len] = '\0';
        parseLineSave(title, url, dir);
    }
    html_parser_cleanup(hsp);
}


void buildWebIndex(char *url){
    const char *dir = "webindex/\0";
    FILE *f_out;
    char *p, f[MAXURL + 10], word[MAXURL];

    p = url;
    while (p != NULL){
        p = GetNextWord(p, word);
        if (strlen(word) > 0 
            && strcmp(word, "http") != 0 
            && strcmp(word, "https") != 0){
            strcpy(f, dir);
            strcat(f, word);
            f_out = fopen(f, "a");
            fprintf(f_out, "%s\n", url);
            fclose(f_out);
        }
    }
}


char *GetLinksFromWebPage(char *myhtmlpage, char *myurl) {
    HTMLSTREAMPARSER *hsp = html_parser_init();

    //htmlparser setup
    char tag[2];
    char attr[5];
    char val[128];
    html_parser_set_tag_to_lower(hsp, 1);
    html_parser_set_attr_to_lower(hsp, 1);
    html_parser_set_tag_buffer(hsp, tag, sizeof(tag));
    html_parser_set_attr_buffer(hsp, attr, sizeof(attr));
    html_parser_set_val_buffer(hsp, val, sizeof(val)-1);
    
    char *urllist = malloc(MAXPAGESIZE * sizeof(char));
    urllist[0] = '\0';
    size_t realsize = strlen(myhtmlpage), p;

    for (p = 0; p < realsize; p++)
    {
        html_parser_char_parse(hsp, ((char *)myhtmlpage)[p]);
        if (html_parser_cmp_tag(hsp, "a", 1))
            if (html_parser_cmp_attr(hsp, "href", 4))
                if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
                {
                    html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
                    if (strstr(html_parser_val(hsp), "http") == NULL){
                        strcat(urllist, myurl);
                    }
                    if (strlen(urllist) + strlen(html_parser_val(hsp)) > MAXPAGESIZE){
                        html_parser_cleanup(hsp);
                        return urllist;
                    }
                    strcat(urllist, html_parser_val(hsp));
                    strcat(urllist, "\n");
                }
    }

    html_parser_cleanup(hsp);

    return urllist;
}


int QSize(char *q) {
    int k, total;
    total = 0;
    for(k = 0; k < MAXQSIZE; k++) {
        if(q[k] == '\n') {
            total++;
        }
        if(q[k] == '\0') {
            return total;
        }
    }
    return total;
}


void AppendLinks(char *p, char *q, char *weblinks) {
    char url[MAXURL], *url_md5, *r;
    int i;
    r = weblinks;
    for (i = 0; i < QSize(weblinks); i++){
        GetNextURL(r, weblinks, url);
        r = ShiftP(r, weblinks);
        url_md5 = str2md5(url, strlen(url));
        if (!find_in_tree(url_md5)) {
            insert_in_tree(url);
            if (strlen(q) + strlen(url) + 1 < MAXQSIZE){
                strcat(p, url);
                strcat(p, "\n");
            } else {
                printf("Queue is full...exiting\n");
                exit(0);
            }
        } 
    }
}


//returns position of the start of the next word
//returns NULL if end of file reached
//function can return words of length zero
char *GetNextWord(char *p, char *myword) {
    const char diff = 'a' - 'A';
    int k;
    for(k = 0; k <= strlen(p); k++) {
        //cut words on these characters
        if(p[k] == '\n' || p[k] == '-'
        || p[k] == '/' || p[k] == '.'
        || p[k] == '?' || p[k] == '='
        || p[k] == ' ' || p[k] == '\t'
        || p[k] == '&' || p[k] == '+'
        || p[k] == ';' || p[k] == ':'
        || p[k] == ',' || p[k] == '@'
        || p[k] == '(' || p[k] == ')'
        || p[k] == '%' || p[k] == '#'
        || (p[k] >= '0' && p[k] <= '9')) {
            myword[k] = '\0';
            p += k + 1;
            return p;
        } else {
            //eliminate upper case
            if (p[k] >= 'A' && p[k] <= 'Z')
                myword[k] = p[k] + diff;
            else
                myword[k] = p[k];
        }
    }
    return NULL;
}


int GetNextURL(char *p, char *q, char *myurl) {
    int i;
    for(i = 0; i < MAXURL; i++) {
        if(p[i] == '\n') {
            myurl[i] = '\0';
            return 1;
        } else {
            myurl[i] = p[i];
        }
    }
    strcpy(myurl,"http://127.0.0.1");
    return 0;
}


char *ShiftP(char *p, char *q) {
  int k;
  for (k = 0; k < MAXURL; k++){
    if(p[k] == '\n'){
        p = p + k + 1;
        return(p);
    }
  }
  return 0;
}

void find_in_tree2(char *url, struct node **par, struct node **loc)
{
    *par = NULL;
    *loc = NULL;
    struct node *ptr,*ptrsave;
    if(url_tree == NULL) 
        return;
    if(!(strcmp(url, url_tree->data)))
    {
        *loc = url_tree;
        return;
    }
    ptrsave = NULL;
    ptr = url_tree;
    while(ptr != NULL) {
        if(!(strcmp(url, ptr->data))) break;
        ptrsave = ptr;
        if(strcmp(url, ptr->data) < 0)
            ptr = ptr->left;
        else
            ptr = ptr->right;
    }
    *loc = ptr;
    *par = ptrsave;
}


int find_in_tree(char *url)
{
    struct node *parent, *location;
    find_in_tree2(url, &parent, &location);
    if (location != NULL)
        return 1;
    else
        return 0;
}


void insert_in_tree(char *url) {
    struct node *parent, *location, *temp;
    find_in_tree2(url, &parent, &location);
    if(location != NULL)
    {
        return;
    }
    temp = malloc(sizeof(struct node));
    temp->data = malloc(MAXURL * sizeof(char));
    strcpy(temp->data, url);
    temp->left = NULL;
    temp->right = NULL;
    if(parent == NULL)
        url_tree = temp;
    else
        if(strcmp(url, parent->data) < 0)
            parent->left = temp;
        else
            parent->right = temp;
}

//simple whitelist function to add allowed sites more easily
int whitelist(char *url) {
    const int wlist_size = 3;
    const char * wlist[wlist_size];
    wlist[0] = "leidenuniv.nl";
    wlist[1] = "liacs.nl";
    wlist[2] = "universiteitleiden.nl"; 
    int i;
    for (i = 0; i < wlist_size; i++)
    {
        if (strstr(url, wlist[i]) != NULL)
            return 1;
    }
    return 0;
}


int main(int argc, char* argv[]) {
    char *url = NULL;
    char *p, *q;
    char urlspace[MAXURL];
    char *html, *weblinks, *imagelinks;
    int k, qs, ql;
    int v;

    q = ((char *) malloc(MAXQSIZE));  // When done, use free(q) to free the memory back to the OS
    if (q==NULL) {
        printf("\n\nProblem with malloc...exiting\n\n"); 
        exit(0);
    }
    q[0]='\0';
    p = q;

    if (argc <= 1) {
        printf("\n\nNo webpage given...exiting\n\n"); 
        exit(0);
    } else { 
        url = argv[1];
        if(strstr(url,"http") != NULL) {
            printf("\nInitial web URL: %s\n\n", url);
        } else {
            printf("\n\nYou must start the URL with lowercase http...exiting\n\n"); 
            exit(0);
        }
    }

    strcat(q, url);  strcat(q, "\n");
    url = urlspace;

    for(k = 0; k < MAXDOWNLOADS; k++) {
        qs = QSize(q); 
        ql = strlen(q);
        printf("\nDownload #: %d   Weblinks: %d   Queue Size: %d\n",k, qs, ql);

        if (!GetNextURL(p, q, url)) {
            printf("\n\nNo URL in queue\n\n");
            exit(0);
        }
        p = ShiftP(p, q);

        //whitelist disabled
        if (1 || whitelist(url)) {
            printf("url=%s\n", url);
            html = GetWebPage(url);

        if (html == NULL) { 
            printf("\n\nhtml is NULL\n\n");
        } 

        if (html) { 
            v = strlen(html); 
            printf("\n\nwebpage size of %s is %d\n\n",url,v);
            weblinks = GetLinksFromWebPage(html, url);
            AppendLinks(p, q, weblinks);
            buildWebIndex(url);
            buildTitleIndex(html, url);
            buildLinkIndex(weblinks, url);
            buildPageIndex(html, url);
            saveWebPage(html, url);
        }
      } else {
          printf("\n\nNot in allowed domains: %s\n\n",url);
      }
    }
    free(q);
}

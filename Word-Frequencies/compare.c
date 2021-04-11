#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include "stringBST.c"
#include "strbuf.c"

#ifndef DEBUG
#define DEBUG 1
#endif

int main(int argc, char* argv[]){
    FILE *fp = fopen("text.txt", "r");
    node* root = NULL;
    int nodeCount = 0;
    strbuf_t word;
    sb_init(&word, 8);
    char c;
    while(1){
        c = fgetc(fp);
        if(c == EOF){
            if(word.used != 1){
                char* temp = malloc(word.used);
                strcpy(temp, word.data);
                root = insert(root, temp);
                nodeCount++;
                sb_destroy(&word);
                break;
            }
        }
        else if(isalpha(c) == 0){
            if(c == ' ' && word.used != 1){
                char* temp = malloc(word.used);
                strcpy(temp, word.data);
                //printf("%s\n",temp );
                root = insert(root, temp);
                nodeCount++;
                sb_destroy(&word);
                sb_init(&word, 8);
            }
        }
        else{
            sb_append(&word, c);
        }
    }
    printTree(root);
    if(DEBUG) printf("\nNode count: %d\n", nodeCount);
    setFrequency(root, nodeCount);
}
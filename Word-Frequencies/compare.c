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
#include <math.h>

#ifndef DEBUG
#define DEBUG 1
#endif

BST* readWords(FILE* fp){
    BST* tree = newBST();
    strbuf_t word;
    sb_init(&word, 8);
    char c;
    
    while(1){
        c = fgetc(fp);
        if(c == EOF){
            if(word.used != 1){
                char* temp = malloc(word.used);
                strcpy(temp, word.data);
                tree->root = insert(tree->root, temp);
                tree->totalCount++;
                sb_destroy(&word);
            }
            break;
        }
        else if(isalpha(c) == 0){
            if(c == ' ' && word.used != 1){
                char* temp = malloc(word.used);
                strcpy(temp, word.data);
                //printf("%s\n",temp );
                tree->root = insert(tree->root, temp);
                tree->totalCount++;
                sb_destroy(&word);
                sb_init(&word, 8);
            }
        }
        else{
            sb_append(&word, tolower(c));
        }
    }
    setFrequency(tree);
    if(tree->root == NULL){
        if(DEBUG) printf("Tree is empty\n");
    }
    
    return tree;
}

int main(int argc, char* argv[]){
    FILE *fp = fopen("text.txt", "r");
    BST* tree = readWords(fp);
    printTree(tree);
    if(DEBUG) printf("\nNode count: %d\n", tree->totalCount);

    node* temp = findWord(tree, "hi");
    printf("\n");
    //printf("\n%f\n", findWord(tree, "hi")->frequency);
    printf("Frequency of '%s': %f\n", temp->word, temp ? temp->frequency : 0);
}
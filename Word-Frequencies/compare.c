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


int main(int argc, char* argv[]){
    FILE *fp = fopen("text.txt", "r");
    node* root;
    strbuf_t* word;
    sb_init(word, 8);
    char c;
    while(1){
        c = fgetc(fp);
        if(c == EOF){
            break;
        }
        else if(c == ' '){
            printList(word);
            insert(root,word->data);
            sb_destroy(word);
            sb_init(word,8);
        }
        else if((isalpha(c) == 0)){
            continue;
        }
        else{
            sb_append(word, c);
        }
    }
    printTree(root);
}


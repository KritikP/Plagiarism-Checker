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
#include <math.h>
#include "stringBST.c"
#include "strbuf.c"
#include "queue.c"

#ifndef DEBUG
#define DEBUG 1
#endif

int processDirs(int* dirThreadNums){

    return (int)* dirThreadNums;
}

int isDir(char *name){
    struct stat data;

	int err = stat(name, &data);
	// should confirm err == 0
	if(err){
	    perror("File does not exist or cannot be opened.\n");
		return EXIT_FAILURE;
    }

    if(S_ISDIR(data.st_mode))
        return 2;
    
    if(S_ISREG(data.st_mode))
        return 3;
    
    return EXIT_FAILURE;
}

BST* readWords(char* name){
    BST* tree = newBST();
    FILE *fp = fopen(name, "r");
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
    int dirNum;
    int directoryThreads = 1;
    int fileThreads = 1;
    int analysisThreads = 1;
    char* suffix = NULL;
    queue_t dirQueue;
    queue_t fileQueue;
    pthread_t* tids;

    for(int i = 1; argv[i] != NULL; i++){
        if(argv[i][0] == '-'){
            char* temp = malloc(strlen(argv[i]) - 1 * sizeof(char));
            for(int j = 2; j < strlen(argv[i]); j++){
                temp[j - 2] = argv[i][j];
            }
            switch((argv[i][1])){
                case ('d') :
                    directoryThreads = atoi(temp);
                    break;
                case('f') :
                    fileThreads = atoi(temp);
                    break;
                case('a') :
                    analysisThreads = atoi(temp);
                    break;
                case('s') :
                    suffix = malloc(strlen(temp) * sizeof(char));
                    strcpy(suffix, temp);
                    break;
            }
            free(temp);
            
        }
        else{
            dirNum = isDir(argv[i]);
            if(dirNum == 2){
                //Is directory
                enqueue(&dirQueue, argv[i]);
            }
            else if(dirNum == 3){
                if(argv[i][0] == '.')
                    break;
                
                //Is valid file
                enqueue(&fileQueue, argv[i]);
            }
        }
    }

    if(!suffix){
        suffix = ".txt";
    }

    int* dirActiveThreads = malloc(sizeof(int) * directoryThreads);
    *dirActiveThreads = directoryThreads;

<<<<<<< HEAD
    while(dequeue(&dirQueue) != NULL){
        char* dirName;
        dirName = malloc(sizeof(strlen(dequeue(&dirQueue))));
        dirName = dequeue(&dirQueue);
=======
    while(dequeue(dirQeue) != NULL){
        char* dirName;
        dirName = malloc(sizeof(stlen(dequeue(dirQueue))));
        dirName = dequeue(dirQueue);
>>>>>>> b9bc337dc44e9b54df75d4fbea3ffbf449553303
        //open dir
        DIR *dir;
        struct dirent *dp;
        dir = opendir(dirName);
        if((dir == NULL)){
            perror("Cannot open");
            exit(1);
        }
        else{
            while((dp = readdir(dir)) != NULL){
<<<<<<< HEAD
                dirNum = isDir(dp->d_name);
=======
                dirNum = isDir(dp);
>>>>>>> b9bc337dc44e9b54df75d4fbea3ffbf449553303
                if(dirNum == 2){
                //Is directory
                enqueue(&dirQueue, dp->d_name);
                }
                else if(dirNum == 3){
<<<<<<< HEAD
                    if(dp->d_name[0] == '.')
=======
                    if(dirName == '.')
>>>>>>> b9bc337dc44e9b54df75d4fbea3ffbf449553303
                        break;
                    
                    //Is valid file
                    enqueue(&fileQueue, dp->d_name);
                }
            }
        }

    }

    printf("Directory threads: %d\nFile threads: %d\nAnalysis threads: %d\nFile name suffix: %s\n",
    directoryThreads, fileThreads, analysisThreads, suffix);

    BST* tree1 = readWords("text.txt");
    BST* tree2 = readWords("text2.txt");
    
    printTree(tree1);
    printf("\n");
    printTree(tree2);
    printf("\n");
    //if(DEBUG) printf("\nNode count: %d\n", tree->totalCount);

    printf("\n");
    printf("JSD: %f\n", getJSD(tree1, tree2));
    //node* temp = findWord(tree, "hi");
    //printf("\n%f\n", findWord(tree, "hi")->frequency);
    //printf("Frequency of '%s': %f\n", temp->word, temp ? temp->frequency : 0);
    
    
}
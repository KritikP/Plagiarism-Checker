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
#include <pthread.h>

#include "stringBST.c"
#include "strbuf.c"
#include "queue.c"

#ifndef DEBUG
#define DEBUG 1
#endif

typedef struct bstLL{
    BST* data;
    struct bstLL* next;
}bstLL;

typedef struct Queues_t{
    queue_t* dirQueue;
    queue_t* fileQueue;
    char* suffix;
}Queues_t;

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

void* processDirs(void* q){
    Queues_t* queues = (Queues_t*) q;
    int dirNum;
    char* dirName;
    strbuf_t filePath;

    printf("Thread:\n");

    while(queues->dirQueue->activeThreads != 0){
        if((dirName = dequeue(queues->dirQueue)) == NULL)
            continue;
        
        printf("dirName: %s\n", dirName);
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
                if(dp->d_name[0] == '.'){
                    continue;
                }
                sb_init(&filePath, 10);
                printf("File in %s: '%s'\n", dirName, dp->d_name);
                sb_concat(&filePath, dirName);
                sb_append(&filePath, '/');
                sb_concat(&filePath, dp->d_name);
                
                printf("File path: %s\n", filePath.data);
                dirNum = isDir(filePath.data);
                printf("dirNum: %d\n", dirNum);
                
                if(dirNum == 2){
                    //Is valid directory
                    printf("Inserting directory path into dirQueue: '%s'\n", filePath.data);
                    enqueue(queues->dirQueue, filePath.data);
                }
                else if(dirNum == 3){

                    bool validSuffix = true;
                    int dpLength = strlen(dp->d_name);
                    int suffixLength = strlen(queues->suffix);
                    printf("dpLength and suffix length: %d, %d\n", dpLength, suffixLength);
                    for(int i = 0; i < suffixLength; i++){
                        if(queues->suffix[i] != dp->d_name[dpLength - suffixLength + i]){
                            validSuffix = false;
                            break;
                        }
                    }

                    //Is valid file
                    if(validSuffix){
                        printf("Inserting file path into fileQueue: '%s'\n", filePath.data);
                        enqueue(queues->fileQueue, filePath.data);
                    }
                }
                sb_destroy(&filePath);
            }
        }
    }
    
    printf("End thread\n");
    return 0;
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

bstLL* insertbstLL(bstLL* head,BST* tree){
    //insert to the front of the LL
    bstLL* insertBST = malloc(sizeof(bstLL));
    insertBST->data = tree;
    insertBST->next = head;
    head = insertBST;
}

int main(int argc, char* argv[]){
    int dirNum;
    int directoryThreads = 1;
    int fileThreads = 1;
    int analysisThreads = 1;
    char* suffix = NULL;
    Queues_t queues;
    queue_t dirQ;
    queue_t fileQ;
    init(&dirQ);
    init(&fileQ);
    queues.dirQueue = &dirQ;
    queues.fileQueue = &fileQ;

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
                enqueue(queues.dirQueue, argv[i]);
            }
            else if(dirNum == 3){
                if(argv[i][0] == '.')
                    break;
                
                //Is valid file
                enqueue(queues.fileQueue, argv[i]);
            }
        }
    }
    
    printf("Initial directory queue count: %d\n", dirQ.count);
    printf("Initial file queue count: %d\n", fileQ.count);
    setThreads(queues.dirQueue, directoryThreads);
    setThreads(queues.fileQueue, fileThreads);
    
    if(!suffix){
        suffix = ".txt";
    }
    queues.suffix = suffix;
    pthread_t d_tids[directoryThreads];
    pthread_t f_tids[fileThreads];
    
    //Start the directory threads
    for(int i = 0; i < directoryThreads; i++){
        pthread_create(&d_tids[i], NULL, processDirs, &queues);
    }

    int* returnVal = 0;
    for(int i = 0; i < directoryThreads; i++){
        pthread_join(d_tids[i], NULL);
        //printf("Return value of thread %ld: %d\n", d_tids[0], *returnVal);
    }
    printf("Post directory threads file queue count: %d\n", fileQ.count);

    printf("Directory threads: %d\nFile threads: %d\nAnalysis threads: %d\nFile name suffix: %s\n",
    directoryThreads, fileThreads, analysisThreads, suffix);
    
    char* fileName;
    while((fileName = dequeue(&fileQ)) != NULL){
        printf("File %d: %s\n", fileQ.count, fileName);
    }

    /*
    BST* tree1 = readWords("text.txt");
    BST* tree2 = readWords("text2.txt");
    
    printTree(tree1);
    printf("\n");
    printTree(tree2);
    printf("\n");
    //if(DEBUG) printf("\nNode count: %d\n", tree->totalCount);

    printf("\n");
    printf("JSD: %f\n", getJSD(tree1, tree2));
    */
    
}
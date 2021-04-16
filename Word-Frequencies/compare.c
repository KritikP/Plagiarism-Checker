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
    bstLL* WFD;
    char* suffix;
}Queues_t;

int isDir(char *name){
    struct stat data;

	int err = stat(name, &data);
	// should confirm err == 0
	if(err){
	    perror("isDir: File does not exist or cannot be opened.\n");
		return EXIT_FAILURE;
    }

    if(S_ISDIR(data.st_mode))
        return 2;
    
    if(S_ISREG(data.st_mode))
        return 3;
    
    return EXIT_FAILURE;
}

bstLL* insertbstLL(bstLL* head, BST* tree){
    //insert to the front of the LL
    bstLL* newLLNode = malloc(sizeof(bstLL));
    newLLNode->data = tree;
    newLLNode->next = NULL;
    
    if(head == NULL){
        head = newLLNode;
        return head;
    }

    bstLL* curr = head;
    //bstLL* prev = curr;

    while(curr->next != NULL){
        //prev = curr;
        curr = curr->next;
    }
    
    curr->next = newLLNode;
    
    return head;
}

BST* readWords(char* name){
    BST* tree = newBST();
    FILE* fp = fopen(name, "r");

    if(fp == NULL){
        printf("File %s couldn't be opened for readWords.\n", name);
        fclose(fp);
        return NULL;
    }
    strbuf_t* word = malloc(sizeof(strbuf_t));
    sb_init(word, 8);
    char c;
    while(1){
        c = fgetc(fp);
        if(c == EOF){
            if(word->used != 1){
                char* temp = malloc(word->used);
                strcpy(temp, word->data);
                tree->root = insert(tree->root, temp);
                tree->totalCount++;
                sb_destroy(word);
            }
            else{
                if(DEBUG) printf("Empty file %s\n", name);
                free(word);
                fclose(fp);
                return NULL;
            }
            break;
        }
        else if(isalpha(c) == 0){
            if(c == ' ' && word->used != 1){
                char* temp = malloc(word->used);
                strcpy(temp, word->data);
                //printf("%s\n",temp );
                tree->root = insert(tree->root, temp);
                tree->totalCount++;
                sb_destroy(word);
                sb_init(word, 8);
            }
        }
        else{
            sb_append(word, tolower(c));
        }
    }
    setFrequency(tree);
    if(tree->root == NULL){
        if(DEBUG) printf("Tree is empty\n");
    }
    free(word);
    fclose(fp);
    return tree;
}

void* processFiles(void* q){
    Queues_t* queues = (Queues_t*) q;
    BST* tempBST;
    char* name;
    //For every file, create the tree and insert into the bst list
    while(queues->fileQueue->activeThreads != 0){
        //printf("\n\nCreating BST for file.\n\n\n");
        name = dequeue(queues->fileQueue);
        if(name != NULL){
            //printf("\nCreating BST for file '%s'.\n", name);
            tempBST = readWords(name);
            if(tempBST == NULL){
                continue;
            }
            printf("\nCreating BST for file '%s'.\n", name);
            queues->WFD = insertbstLL(queues->WFD, tempBST);

        }
    }
    
    return 0;
}

void* processDirs(void* q){
    Queues_t* queues = (Queues_t*) q;
    int dirNum;
    char* dirName;
    strbuf_t filePath;

    if(DEBUG)printf("Thread:\n");

    while(queues->dirQueue->activeThreads != 0){
        if((dirName = dequeue(queues->dirQueue)) == NULL)
            continue;
        
        //printf("dirName: %s\n", dirName);
        //open dir
        DIR *dir;
        struct dirent *dp;
        dir = opendir(dirName);
        
        if((dir == NULL)){
            perror("Cannot open dir\n");
            exit(1);
        }
        else{
            while((dp = readdir(dir)) != NULL){
                if(dp->d_name[0] == '.'){
                    continue;
                }
                sb_init(&filePath, 100);
                sb_concat(&filePath, dirName);
                sb_append(&filePath, '/');
                sb_concat(&filePath, dp->d_name);
                
                //printf("File path: %s\n", filePath.data);
                dirNum = isDir(filePath.data);
                //printf("dirNum: %d\n", dirNum);
                
                if(dirNum == 2){
                    //Is valid directory
                    if(DEBUG)printf("Inserting directory path into dirQueue: '%s'\n", filePath.data);
                    enqueue(queues->dirQueue, filePath.data);
                }
                else if(dirNum == 3){

                    bool validSuffix = true;
                    int dpLength = strlen(dp->d_name);
                    int suffixLength = strlen(queues->suffix);
                    for(int i = 0; i < suffixLength; i++){
                        if(queues->suffix[i] != dp->d_name[dpLength - suffixLength + i]){
                            validSuffix = false;
                            break;
                        }
                    }

                    //Is valid file
                    if(validSuffix){
                        if(DEBUG)printf("Inserting file path into fileQueue: '%s'\n", filePath.data);
                        enqueue(queues->fileQueue, filePath.data);
                    }
                }
                sb_destroy(&filePath);
            }
        }
    }
    
    if(DEBUG)printf("End thread\n");
    return 0;
}

void* printLLBST(bstLL* head){
    bstLL* curr = head;
    if(curr == NULL){
        printf("Empty LL trees.\n");
        return 0;
    }

    while(curr != NULL){
        printTree(curr->data);
        printf("\n");
        curr = curr->next;
    }

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
    queues.WFD = NULL;

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
    
    if(DEBUG) printf("Initial directory queue count: %d\n", dirQ.count);
    if(DEBUG) printf("Initial file queue count: %d\n", fileQ.count);
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

    for(int i = 0; i < directoryThreads; i++){
        pthread_join(d_tids[i], NULL);
    }

    if(DEBUG)printf("Post directory threads file queue count: %d\n", fileQ.count);
    for(int i = 0; i < fileThreads; i++){
        pthread_create(&f_tids[i], NULL, processFiles, &queues);
    }

    for(int i = 0; i < fileThreads; i++){
        pthread_join(f_tids[i], NULL);
    }

    printf("Directory threads: %d\nFile threads: %d\nAnalysis threads: %d\nFile name suffix: %s\n",
    directoryThreads, fileThreads, analysisThreads, suffix);

    printLLBST(queues.WFD);

}
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
    char* fileName;
    struct bstLL* next;
    int fileCount;
}bstLL;

typedef struct JSD_t{
    bstLL* file1;
    bstLL* file2;
    double JSD;
    int totalWordCount;
}JSD_t;

typedef struct JSDList{
    JSD_t** JSDs;
    int totalAnalThreads;
    int currentAnalThreads;
    int totalComparisons;
    int evenComparisons;
    int extraComparisons;
	pthread_mutex_t lock;
}JSDList;

typedef struct Queues_t{
    queue_t* dirQueue;
    queue_t* fileQueue;
    bstLL* WFD;
    char* suffix;
}Queues_t;

void freeBSTLL(bstLL* root){
    bstLL* curr = root;
    bstLL* prev = curr;

    while(curr != NULL){
        curr = curr->next;
        freeBST(prev->data);
        free(prev->fileName);
        free(prev);
        prev = curr;
    }

}

void freeJSDList(JSDList* j){
    for(int i = 0; i < j->totalComparisons; i++){
        free(j->JSDs[i]);
    }
    pthread_mutex_destroy(&(j->lock));
    free(j);
}

void JSDListInit(JSDList* JSDL, int totalComps, int threadNums, JSD_t** jsds){
    JSDL->JSDs = jsds;
    JSDL->totalAnalThreads = threadNums;
    JSDL->currentAnalThreads = 0;
    JSDL->totalComparisons = totalComps;
    JSDL->evenComparisons = totalComps / threadNums;
    JSDL->extraComparisons = totalComps % threadNums;
	pthread_mutex_init(&JSDL->lock, NULL);
}

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

bstLL* insertbstLL(bstLL* head, BST* tree, char* name){
    //insert to the front of the LL
    bstLL* newLLNode = malloc(sizeof(bstLL));
    newLLNode->data = tree;
    newLLNode->next = NULL;
    newLLNode->fileName = name;
    
    if(head == NULL){
        head = newLLNode;
        head->fileCount = 1;
        return head;
    }

    bstLL* curr = head;

    while(curr->next != NULL){
        curr = curr->next;
    }
    
    curr->next = newLLNode;
    head->fileCount++;
    return head;
}

BST* readWords(char* name){
    BST* tree = newBST();
    FILE* fp = fopen(name, "r");

    if(fp == NULL){
        perror("readWords: File couldn't be opened: ");
        //write(2, name, strlen(name));
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
                //printf("%s\n", temp);
                tree->root = insert(tree->root, word->data);
                tree->totalCount++;
                sb_destroy(word);
                free(word);
                fclose(fp);
            }
            else{
                if(DEBUG) printf("Empty file %s\n", name);
                sb_destroy(word);
                free(word);
                fclose(fp);
            }
            break;
        }
        else if(isalpha(c) == 0 && c != '-' && isdigit(c) == 0){
            if((isspace(c) != 0) && word->used != 1){
                //printf("%s\n", temp);
                tree->root = insert(tree->root, word->data);
                tree->totalCount++;
                sb_destroy(word);
                sb_init(word, 8);
            }
        }
        else{
            sb_append(word, tolower(c));
        }
    }
    
    if(tree->root == NULL){
        if(DEBUG) printf("Tree is empty, so file was empty\n");
        freeBST(tree);
        return NULL;
    }
    else
        setFrequency(tree);

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
            if(tempBST == NULL){        //If the tree is NULL, the file was empty and no words were added
                continue;
            }
            queues->WFD = insertbstLL(queues->WFD, tempBST, name);

        }
    }
    
    return 0;
}

void* processDirs(void* q){
    Queues_t* queues = (Queues_t*) q;
    int dirNum;
    char* dirName;
    strbuf_t* filePath = malloc(sizeof(strbuf_t));
    if(DEBUG)printf("Thread:\n");

    while(queues->dirQueue->activeThreads != 0){
        dirName = dequeue(queues->dirQueue);
        if(dirName == NULL)
            continue;
        
        //printf("dirName: %s\n", dirName);
        //open dir
        DIR *dir;
        struct dirent *dp;
        dir = opendir(dirName);
        
        if((dir == NULL)){
            printf("Cannot open dir\n");
            perror("Cannot open dir\n");
            exit(1);
        }
        else{
            while((dp = readdir(dir)) != NULL){
                if(dp->d_name[0] == '.'){
                    continue;
                }
                sb_init(filePath, 100);
                sb_concat(filePath, dirName);
                sb_append(filePath, '/');
                sb_concat(filePath, dp->d_name);
                
                //printf("File path: %s\n", filePath.data);
                dirNum = isDir(filePath->data);
                //printf("dirNum: %d\n", dirNum);
                
                if(dirNum == 2){
                    //Is valid directory
                    if(DEBUG)printf("Inserting directory path into dirQueue: '%s'\n", filePath->data);
                    enqueue(queues->dirQueue, filePath->data);
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
                        if(DEBUG)printf("Inserting file path into fileQueue: '%s'\n", filePath->data);
                        enqueue(queues->fileQueue, filePath->data);
                    }
                }
                sb_destroy(filePath);
            }
        }
        free(dirName);
        closedir(dir);
    }
    
    free(filePath);
    if(DEBUG)printf("End thread\n");
    return 0;
}

void* processAnal(void* jsd){
    JSDList* JSDL = (JSDList*) jsd;
    int startIndex;
    int endIndex;

    pthread_mutex_lock(&JSDL->lock);
    
    if(DEBUG)printf("Curr anal, totalAnal, evenComps,: %d, %d, %d\n", JSDL->currentAnalThreads, JSDL->totalAnalThreads, JSDL->evenComparisons);
    if(JSDL->currentAnalThreads != JSDL->totalAnalThreads - 1){
        startIndex = JSDL->currentAnalThreads * JSDL->evenComparisons;
        endIndex = startIndex + JSDL->evenComparisons;
    }
    else{
        startIndex = JSDL->currentAnalThreads * JSDL->evenComparisons;
        endIndex = JSDL->currentAnalThreads * JSDL->evenComparisons +
            JSDL->totalComparisons % JSDL->totalAnalThreads + JSDL->evenComparisons;
        
    }
    JSDL->currentAnalThreads++;
    pthread_mutex_unlock(&JSDL->lock);

    if(DEBUG)printf("Start and end index: %d, %d\n", startIndex, endIndex);
    
    while(startIndex < endIndex){
        if(DEBUG) printf("JSD comparison %d\n", startIndex);
        JSDL->JSDs[startIndex]->JSD = 1;
        JSDL->JSDs[startIndex]->JSD = getJSD(JSDL->JSDs[startIndex]->file1->data, JSDL->JSDs[startIndex]->file2->data);
        startIndex++;
    }

    return 0;
}

void* printLLBST(bstLL* head){
    bstLL* curr = head;
    if(curr == NULL){
        if(DEBUG)printf("Empty LL trees.\n");
        return 0;
    }

    while(curr != NULL){
        if(DEBUG){
            printf("%s: ", curr->fileName);
            printTree(curr->data);
            printf("\n");
        }
        curr = curr->next;
    }

}

void createJSDs(JSD_t** jsds, bstLL* head){
    bstLL* start = head;
    bstLL* curr = head->next;
    int i = 0;
    
    while(start != NULL){
        while(curr != NULL){
            if(DEBUG)printf("Make jsd at i: %d\n", i);
            jsds[i] = malloc(sizeof(JSD_t));
            jsds[i]->file1 = start;
            jsds[i]->file2 = curr;
            jsds[i]->totalWordCount = start->data->totalCount + curr->data->totalCount;
            curr = curr->next;
            i++;
        }
        start = start->next;
        if(start != NULL)
            curr = start->next;
    }
}

int cmpfunc(const void* a, const void* b){
    double l = (*(JSD_t**)a)->totalWordCount;
    double r = (*(JSD_t**)b)->totalWordCount;
    if(DEBUG){
        printf("File set 1: %s, %s\t%d, %d\n", (*(JSD_t**)a)->file1->fileName, (*(JSD_t**)a)->file2->fileName
            , (*(JSD_t**)a)->file1->data->totalCount, (*(JSD_t**)a)->file2->data->totalCount);
        printf("File set 2: %s, %s\t%d, %d\n", (*(JSD_t**)b)->file1->fileName, (*(JSD_t**)b)->file2->fileName
            , (*(JSD_t**)b)->file1->data->totalCount, (*(JSD_t**)b)->file2->data->totalCount);
        printf("Count of a and b: %f, %f\n", l, r);
    }
    if(l - r > 0)
        return -1;
    else if(l - r < 0)
        return 1;
    else
        return 0;
}

int main(int argc, char* argv[]){

    if(argc == 1){
        return EXIT_FAILURE;
    }

    int dirNum;
    int directoryThreads = 1;
    int fileThreads = 1;
    int analysisThreads = 1;
    char* suffix = ".txt";
    bool freeSuf = false;
    Queues_t* queues = malloc(sizeof(Queues_t));
    queue_t* dirQ = malloc(sizeof(queue_t));
    queue_t* fileQ = malloc(sizeof(queue_t));
    queues->WFD = NULL;

    init(dirQ);
    init(fileQ);
    queues->dirQueue = dirQ;
    queues->fileQueue = fileQ;

    for(int i = 1; argv[i] != NULL; i++){
        if(argv[i][0] == '-'){
            char* temp = calloc((strlen(argv[i]) - 1), sizeof(char));
            for(int j = 2; j < strlen(argv[i]); j++){
                temp[j - 2] = argv[i][j];
            }
            switch(tolower((argv[i][1]))){
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
                    freeSuf = true;
                    strcpy(suffix, temp);
                    break;
            }
            free(temp);
            
        }
        
        else{
            dirNum = isDir(argv[i]);
            if(dirNum == 2){
                //Is directory
                enqueue(queues->dirQueue, argv[i]);
            }
            else if(dirNum == 3){
                if(argv[i][0] == '.')
                    break;
                
                //Is valid file
                enqueue(queues->fileQueue, argv[i]);
            }
        }
    }
    
    if(DEBUG) printf("Initial directory queue count: %d\n", dirQ->count);
    if(DEBUG) printf("Initial file queue count: %d\n", fileQ->count);
    setThreads(queues->dirQueue, directoryThreads);
    setThreads(queues->fileQueue, fileThreads);

    queues->suffix = suffix;
    pthread_t d_tids[directoryThreads];
    pthread_t f_tids[fileThreads];
    pthread_t a_tids[analysisThreads];
    
    //Start the directory threads
    for(int i = 0; i < directoryThreads; i++){
        pthread_create(&d_tids[i], NULL, processDirs, queues);
    }

    for(int i = 0; i < directoryThreads; i++){
        pthread_join(d_tids[i], NULL);
    }

    if(DEBUG)printf("Post directory threads file queue count: %d\n", fileQ->count);
    for(int i = 0; i < fileThreads; i++){
        pthread_create(&f_tids[i], NULL, processFiles, queues);
    }

    for(int i = 0; i < fileThreads; i++){
        pthread_join(f_tids[i], NULL);
    }

    printf("Directory threads: %d\nFile threads: %d\nAnalysis threads: %d\nFile name suffix: %s\n\n",
    directoryThreads, fileThreads, analysisThreads, suffix);

    if(queues->WFD == NULL){
        write(2, "No files\n", 9);
        if(freeSuf)
            free(suffix);
        free(queues);
        destroy(dirQ);
        destroy(fileQ);
        free(dirQ);
        free(fileQ);
        return EXIT_FAILURE;
    }
    int fileNums = queues->WFD->fileCount;
    if(fileNums < 2){
        write(2, "Less than 2 files\n", 18);
        if(freeSuf)
            free(suffix);
        freeBSTLL(queues->WFD);
        free(queues);
        destroy(dirQ);
        destroy(fileQ);
        free(dirQ);
        free(fileQ);

        return EXIT_FAILURE;
    }

    printLLBST(queues->WFD);
    printf("Total file nums: %d\n", fileNums);
    int comparisons = 0.5 * fileNums *(fileNums - 1);
    JSD_t* jsds[comparisons];
    createJSDs(jsds, queues->WFD);

    JSDList* JSDL = malloc(sizeof(JSDList));
    JSDListInit(JSDL, comparisons, analysisThreads, jsds);

    int ts = analysisThreads;
    if(comparisons < analysisThreads)
        ts = comparisons;
    
    for(int i = 0; i < ts; i++){
        pthread_create(&a_tids[i], NULL, processAnal, JSDL);
    }

    for(int i = 0; i < ts; i++){
        pthread_join(a_tids[i], NULL);
    }

    qsort(jsds, comparisons, sizeof(JSD_t*), cmpfunc);

    for(int i = 0; i < comparisons; i++){
        printf("%f %s %s\n", jsds[i]->JSD, jsds[i]->file1->fileName, jsds[i]->file2->fileName);
        //printTree(jsds[i]->file1->data);
        //printf("\n");
        //printTree(jsds[i]->file2->data);
        //printf("\n");
        //printf("Total word in jsd: %d\n", jsds[i]->totalWordCount);
    }

    if(freeSuf)
        free(suffix);
    freeJSDList(JSDL);
    freeBSTLL(queues->WFD);
    free(queues);
    destroy(dirQ);
    destroy(fileQ);
    free(dirQ);
    free(fileQ);
    
}
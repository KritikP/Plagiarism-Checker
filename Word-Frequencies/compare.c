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

#ifndef DEBUG
#define DEBUG 1
#endif

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

float KLDHelper(node* root, BST* meanTree){
    
    if(root != NULL){
        return root->frequency * log2f(root->frequency / findWord(meanTree, root->word)->frequency)
        + KLDHelper(root->leftChild, meanTree) + KLDHelper(root->rightChild, meanTree);
    }
    else{
        return 0;
    }

}

float getKLD(BST* tree, BST* meanTree){
    return KLDHelper(tree->root, meanTree);
}

float getJSD(BST* tree1, BST* tree2){
    BST* meanTree = meanFrequencyTree(tree1, tree2);
    return sqrt(0.5f * getKLD(tree1, meanTree) + 0.5f * getKLD(tree2, meanTree));
}

int main(int argc, char* argv[]){
    int dirNum;
    for(int i = 1; argv[i] != NULL; i++){
        dirNum = isDir(argv[i]);
        if(dirNum == 2){
            //Is directory
        }
        else if(dirNum == 3){
            //Is file
        }
    }

    FILE *fp = fopen("text.txt", "r");
    BST* tree1 = readWords(fp);

    FILE *fp2 = fopen("text2.txt", "r");
    BST* tree2 = readWords(fp2);
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
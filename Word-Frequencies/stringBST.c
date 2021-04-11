#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node{
    int count;
    int occurrence;
    char* word;
    struct node* leftChild;
    struct node* rightChild;
} node;

node* findWord(node* root, char* word){
    if(root == NULL || strcmp(word, root->word) == 0)
        return root;
    else if(strcmp(word, root->word) < 0)                   //If the search word is less than the current node, continue search left
        findWord(root->leftChild, word);
    else                                //If the search word is greater than the current node, continue search right
        findWord(root->rightChild, word);
}

node* newNode(char* word){
    node* newNode;
    newNode = malloc(sizeof(node));
   // newNode->word = malloc(sizeof(strlen(word)));
    newNode->word = word;
    newNode->leftChild = NULL;
    newNode->rightChild = NULL;
    return newNode;
}

node* insert(node* root, char* word){
    if(root == NULL)
        return newNode(word);
    else if(strcmp(word, root->word) == 0){     //If the word already exists, just increase the count by one
        root->count++;
    }
    else if(strcmp(word, root->word) < 0){
        root->leftChild = insert(root->leftChild, word);
    }
    else{
        root->rightChild = insert(root->rightChild, word);
    }
    return root;
}

void printTree(node* root){
    if(root != NULL){
        printTree(root->leftChild);
        printf("%s ", root->word);
        printTree(root->rightChild);
    }
}


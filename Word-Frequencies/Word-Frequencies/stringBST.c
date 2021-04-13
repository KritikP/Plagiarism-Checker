#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node{
    int count;
    double frequency;
    char* word;
    struct node* leftChild;
    struct node* rightChild;
} node;

node* newNode(char* word){
    node* newNode;
    newNode = malloc(sizeof(node));
    newNode->word = word;
    newNode->count = 1;
    newNode->frequency = 0;
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

typedef struct BST{
    int totalCount;
    struct node* root;
} BST;

BST* newBST(){
    BST* tree = malloc(sizeof(BST));
    tree->root = NULL;
    tree->totalCount = 0;
    return tree;
}

void printTreeHelper(node* root){
    if(root != NULL){
        printTreeHelper(root->leftChild);
        printf("%s (%f) ", root->word, root->frequency);
        printTreeHelper(root->rightChild);
    }
}

void printTree(BST* tree){
    printTreeHelper(tree->root);
}

void setFrequencyHelper(node* root, int total){
    
    if(root != NULL){
        setFrequencyHelper(root->leftChild, total);
        root->frequency = root->count / (double) total;
        setFrequencyHelper(root->rightChild, total);
    }
    
}

void setFrequency(BST* tree){
    
    setFrequencyHelper(tree->root, tree->totalCount);
    
}

node* findWordHelper(node* root, char* word){
    if(root == NULL || strcmp(word, root->word) == 0)
        return root;
    else if(strcmp(word, root->word) < 0)                   //If the search word is less than the current node, continue search left
        findWordHelper(root->leftChild, word);
    else                                //If the search word is greater than the current node, continue search right
        findWordHelper(root->rightChild, word);
}

node* findWord(BST* tree, char* word){
    return findWordHelper(tree->root, word);
}

void meanFrequencyTreeHelper(node* root1, node* root2, BST* tree){
    node* temp;
    node* temp2;
    if(root1 != NULL){
        meanFrequencyTreeHelper(root1->leftChild, root2, tree);

        temp = findWord(tree, root1->word);
        if(temp == NULL){                       //If the word isn't in the mean tree, add it and calculate the mean freq
            
            tree->root = insert(tree->root, root1->word);     //Insert the word in the mean tree
            temp = findWord(tree, root1->word);       //Find the word that was just inserted in the mean tree

            temp2 = findWordHelper(root2, root1->word);      //Try to get the word in tree 2
            if(temp2 != NULL){                              //If the word is in tree 2
                temp->frequency = 0.5f*(root1->frequency + temp2->frequency);
            }
            else
                temp->frequency = 0.5f*(root1->frequency);
        }

        meanFrequencyTreeHelper(root1->rightChild, root2, tree);
        
    }
    
}

BST* meanFrequencyTree(BST* tree1, BST* tree2){
    BST* tree = newBST();
    meanFrequencyTreeHelper(tree1->root, tree2->root, tree);
    meanFrequencyTreeHelper(tree2->root, tree1->root, tree);
    return tree;
}
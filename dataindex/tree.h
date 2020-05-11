#ifndef __tree_h__
#define __tree_h__

typedef struct List {
	int record_num;
	char* record;
	struct List *next;
}*Dup;

typedef struct node_t {
        int height;
        struct node_t *left;
        struct node_t *right;
	Dup dups;
        char* record;
        int record_num;
	int count;
}*node;

void setField(int);

Dup makeDuplicate(int,char*);

node makeNode(char*,int);

int compare(char*,char*);

int height(node);

int max(int,int);

node rightRotate(node);

node leftRotate(node);

node leftRightRotate(node);

node rightLeftRotate(node);

int getBalance(node);

node insert(node,char*,int);

node search(node, char*);

void deleteTree(node);

void pre(node);

void in(node);

#endif

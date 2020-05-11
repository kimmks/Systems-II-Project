#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

/*
	Create and return a new node
	with the given record value

	add members to work w/ indexed CSV files
*/

int fieldType;

void setField(int type) {
	if(type <=3) {
		fieldType = 0; // char* type
	}
	if(type > 3) {
		fieldType = 1; // char* but numbers type ( "20" )
	}
}

Dup makeDuplicate(int record_num, char* rec) {
	Dup dups = (Dup) malloc(sizeof(struct List));
	dups->record_num = record_num;
	dups->next = NULL;
	dups->record = rec;
        return dups;
}

node makeNode(char* rec, int record_num) {

	//allocate memory for each new node
	node treeNode = (node) malloc(sizeof(struct node_t));

	if(treeNode == NULL) {
		return NULL;
	}

	//initialize fields
	treeNode->height = 1;
	treeNode->record = rec;
	treeNode->left = NULL;
	treeNode->right = NULL;
	treeNode->dups = NULL;
	treeNode->record_num = record_num;

	return treeNode;
}

/*
	Compares two record instances. Takes three parameters: two void
	pointers and an int value used to determine the type of records
	being compares (either char* or int)

	return -1 if recordOne < recordTwo
	return 0 if recordOne = recordTwo --> insert doesn't allow duplicates
	return 1 if recordOne > recordTwo

*/
int compare(char* recordOne, char* recordTwo){

        int ret;
        int cmp;

        int len1 = strlen(recordOne);
        int len2 = strlen(recordTwo);

	char rec1[len1+1];
        char rec2[len2+1];

        strcpy(rec1, recordOne);
        strcpy(rec2, recordTwo);

        cmp = strcmp(rec1, rec2);

        if(cmp < 0) {           // rec1 < rec2
       	    ret = -1;
        } else if(cmp > 0) {    //rec1 > rec2
            ret = 1;
        } else {		// rec1 == rec2
            ret = 0;
        }

        return ret;
}


/*
	Utility function for node heights (used for balance factors)
*/
int height(node treeNode) {

	if(treeNode==NULL) {
		return 0;
	}
	return treeNode->height;
}

/*
	Utility function for balance factors
*/
int max(int left, int right) {
	return (left > right) ? left : right;
}

/*
	right rotate subtree rooted at n
*/
node rightRotate(node n) {

	node x = n->left;
	node z = x->right;

	//rotate
	x->right = n;
	n->left = z;

	//update heights
	n->height = max(height(n->left), height(n->right))+1;
	x->height = max(height(x->left), height(x->right))+1;

	//return new root
	return x;
}

/*
	left rotate subtree rooted at n
*/
node leftRotate(node n) {

	node y = n->right;
	node z = y->left;

	//rotate
	y->left = n;
	n->right = z;

	//update heights
	n->height = max(height(n->left), height(n->right))+1;
        y->height = max(height(y->left), height(y->right))+1;

	//return new root
	return y;
}


/*
	utility function for performing rotations
*/
int getBalance(node n) {
	if(n==NULL) {
		return 0;
	}
	return height(n->left) - height(n->right);
}

/*
	recursively insert a key (record)
	in the subtree rooted at Node n, returning the new root
*/
node insert(node n, char* key, int record_num) {


	//values returned by compare() function
	// key < record --> -1
	// key > record --> 1
	// key == record --> 0 *UNIMPLIMENTED*

	int cmp; // compares key & n->record
	int cmpll; // compares key & n->left->record
	int cmprr; // compares key & n->right->record

	// case 1:  new node needs to be inserted
	if(n == NULL) {
		return(makeNode(key, record_num));
	}

	// different compare used for different types of records
  	if (fieldType == 1) {
    		cmp = atof(key) - atof(n->record);
  	} else {
	 	cmp = compare(key, n->record);
  	}

	// case 2: n is not null, insert key into tree
	if(cmp < 0) {
		n->left = insert(n->left, key, record_num);
	} else if(cmp > 0) {
		n->right = insert(n->right, key, record_num);
	} else {
		Dup newDup = makeDuplicate(record_num, key);
		if (n->dups == NULL) {
			n->dups = newDup;
		} else {
			newDup->next = n->dups;
			n->dups = newDup;
		}

	}

	//update height
	n->height = 1 + max(height(n->left), height(n->right));

	//get balance factor
	int balance = getBalance(n);

        
	// 4 cases of imbalanced trees, perform rotations for each
	if(balance > 1) {
		  if (fieldType == 1) {
    			cmpll = atof(key) - atof(n->left->record);
  		} else {
			cmpll = compare(key, n->left->record);
  		}

		//left-left
		if(cmpll < 0) {
			return rightRotate(n);
		}
		//left-right
		if(cmpll > 0) {
			n->left = leftRotate(n->left);
			return rightRotate(n);
		}
          printf("Balancing tree.\n");
	}
	if(balance < -1) {
		  if (fieldType == 1) {
    			cmprr = atof(key) - atof(n->right->record);
  		  } else {
			cmprr = compare(key, n->right->record);
  		  }

		//right-right
		if(cmprr > 0) {
			return leftRotate(n);
		}
		//right-left 
		if(cmprr < 0) {
			n->right = rightRotate(n->right);
			return leftRotate(n);
		}
	   printf("Balancing tree.\n");
	}
	// return unedited node
	return n;
}

/*
	search a tree rooted at node n for key, return the node
	its found in
*/
node search(node n, char* key) {

	int cmp;

	if(n == NULL) return NULL;

	if (fieldType == 1) {
		cmp = atof(key) - atof(n->record);
  	} else {
		cmp = compare(key, n->record);
	}
	if(cmp == 0) {
		return n;
	} else if(cmp < 0) {
		return search(n->left, key);
	} else {
		return search(n->right, key);
	}
}

/*
	write a tree to an output file

void fileOut(node root, FILE* fp) {
	Dup temp;
	if(root != NULL) {
		fileOut(root->left, fp);
		fprintf(fp, "%s %d ", root->record, root->record_num);
		    if(root->dups != NULL) {
		  temp = root->dups;
			while(temp->next != NULL) {
			fprintf(fp, "%s %d ", temp->record, temp->record_num);
			temp = temp->next;
		        }
		}
		fileOut(root->right, fp);
	}
}
*/
/*
	delete a tree rooted at Node n
*/
void deleteTree(node n) {

	if(n == NULL) return;

	if(n->left != NULL) {
		deleteTree(n->left);
	}
	if(n->right!= NULL) {
		deleteTree(n->right);
	}

	free(n);
}

/*
	preorder traversal
*/
void pre(node root) {
	if(root != NULL) {
		printf("%s\n", root->record);
		pre(root->left);
		pre(root->right);
  	}
}

/*
	inorder traversal
*/
void in(node root) {
	if(root != NULL) {
		in(root->left);
		printf("%s ", root->record);
		if(root->dups != NULL) {
			Dup curDup = root->dups;
			while (curDup!=NULL) {
				printf("%s ", curDup->record);
				curDup = curDup->next;
			}
		}
		in(root->right);
	}
}

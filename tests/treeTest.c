#include "../dataindex/tree.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

char * rec1 = "testing";
char * rec2 = "trees";

node makeTree() {

	node n;
	for(int i = 0; i < 11; i++) {
		n = insert(n, "a" + i, i);

	}
	return n;

}
bool testCompare() {

	int res;
	bool ret;

	res = compare(rec1, rec2);

	if(res == 1) {
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

bool testInsert() {

	node n = makeTree();

	if(n == NULL) {
		return false;
	}

	return true;
}

bool testSearch() {
	node n = makeTree();
	node found = search(n, "a");

	if(found == NULL) {
		return false;
	}
	if(found->record != "a") {
		return false;
	}

	return true;
}

bool testDeleteTree() {

	node n  = makeTree();
	deleteTree(n);
	if(n != NULL) {
		return false;
	}

        return true;
}


void main() {

	bool returned;

	returned = testDeleteTree();

	if(returned == false) {
		printf("Error");
	}

	returned = testInsert();

        if(returned == false) {
                printf("Error");
        }

	returned = testCompare();

        if(returned == false) {
                printf("Error");
        }

	returned = testSearch();

        if(returned == false) {
                printf("Error");
        }

}

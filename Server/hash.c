#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char* doHash(char* data, size_t dataLen) {
    //create pipe
    int child2parent[2];
    pipe(child2parent);

    //create a file for hashing the data
    char* fileName = "temp";
    FILE *file = fopen(fileName, "w");
    //write the data to the file
    fwrite(data, 1, dataLen, file);	
    fclose(file);

    //allocate space for the hash; to be returned
    char* localHash = (char *) malloc(sizeof(char) * 64);

    //for the process
    int pid = fork();
    if (pid == 0) {//child
        //redirect stdout to the pip
        //process thinks it's going to stdout but actually to child2parent
        dup2(child2parent[1], STDOUT_FILENO);
        close(child2parent[0]);
        close(child2parent[1]);

        //gotta exec with the stdin from before test | sha256sum
        char *arguments[] = {"sha256sum", fileName, NULL};
        //hash
        if (execvp("sha256sum", arguments) == -1) {
            printf("fail exec\n");
            abort();
        }
        abort();
    } else {//parent
        //read from the created file with the data
        read(child2parent[0], localHash, 64);
    }
    //wait for the child process to finish
    wait(NULL);
    //delete the new file
    remove(fileName);
    return localHash;
}


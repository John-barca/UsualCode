#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

int main () {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    return 0;
}
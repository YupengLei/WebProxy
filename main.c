/*
 Copyright @ Yupeng Lei, Vincent Chen
 
 Licensed under the MIT License
 
 This is a web proxy in C 
*/

#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define MAXSIZEOFREQUEST 65535
#define MAXIPADDRSIZE 15

struct sockaddr_in myAddr, clientAddr, proxAddr;
long MAXSIZEOFRESPONSE = 2000000;

void errorMes(int statusCode, int clientSocket) {
    char errorHead[MAXSIZEOFREQUEST], errorMessage[MAXSIZEOFREQUEST], htmlResponse[MAXSIZEOFRESPONSE], errorResponse[MAXSIZEOFREQUEST];
    //BLOCKED SITE
    if (statusCode == 403) {
        strcpy(errorHead, "Forbidden");
        strcpy(errorMessage, "You do not have access rights to the content");
    } else if (statusCode == 411) {
        strcpy(errorHead, "Length Required");
        strcpy(errorMessage, "Content-Length header field is not defined");
    } else if (statusCode == 431) {
        strcpy(errorHead, "Request Header Field Too Large");
        strcpy(errorMessage, "The server is unwilling to process the request because its header fields are too large");
    } else if (statusCode == 400) {
        strcpy(errorHead, "Bad Request");
        strcpy(errorMessage, "Could not understand the request due to invalid syntax");
    } else if (statusCode == 404){
        strcpy(errorHead, "Not Found");
        strcpy(errorMessage, "URL not recognized");
    }
    snprintf(htmlResponse, sizeof(htmlResponse), "<html>\n<head>\n<title>%i %s</title>\n</head>\n"
                                                 "<body>\n<h1>%i %s</h1>\n"
                                                 "<p>%s</p>\n</body>\n</html>", statusCode, errorHead, statusCode, errorHead, errorMessage);

    snprintf(errorResponse, sizeof(errorResponse), "HTTP/1.1 %i %s\r\n"
                                                   "Content-Type: text/html\r\n"
                                                   "Content-Length: %li\r\n"
                                                   "\n"
    , statusCode, errorHead,strlen(htmlResponse));
    strcat(errorResponse, htmlResponse);
    write(clientSocket, errorResponse, sizeof(errorResponse));
    close(clientSocket);
    memset(errorHead, 0, MAXSIZEOFREQUEST);
    memset(errorMessage, 0, MAXSIZEOFREQUEST);
    memset(htmlResponse, 0, MAXSIZEOFRESPONSE);
    memset(errorResponse, 0, MAXSIZEOFREQUEST);
}

void doParse(char* httpReq, char* urlptr, char* pathptr, char* portptr) {
    char* tempURL;
    char* tempPATH;
    char* tempPortNumber;
    char* tempPortUrl;
    char holdURL[MAXSIZEOFREQUEST], holdPATH[MAXSIZEOFREQUEST], urlWithPort[MAXSIZEOFREQUEST];
    //Parse out the url
    tempURL = strstr(httpReq, "Host: ");
    tempURL = strchr(tempURL, ' ');
    tempURL = tempURL + 1;
    strcpy(holdURL, tempURL);
    tempURL = strstr(holdURL, "\r");
    *tempURL = 0;
    tempPortNumber = strchr(holdURL, ':');
    if (tempPortNumber == NULL) {
        strcpy(urlptr, holdURL);
    } else {
        strcpy(urlWithPort, holdURL);
        //get the url without PortNumber
        tempPortUrl = strchr(urlWithPort, ':');
        *tempPortUrl = 0;
        strcpy(urlptr, urlWithPort);
        tempPortNumber = tempPortNumber + 1;
        strcpy(portptr, tempPortNumber);
    }

    //parse out the path
    tempPATH = strstr(httpReq, "GET ");
    tempPATH = strchr(httpReq, ' ');
    tempPATH = tempPATH + 8;
    tempPATH = strchr(tempPATH, '/');
    strcpy(holdPATH, tempPATH);
    tempPATH = strstr(holdPATH, " HTTP/");
    *tempPATH = 0;
    strcpy(pathptr, holdPATH);
}

int dns(char *urlHostName, char* ipAddrHost) {
    struct addrinfo hints, *IP_addr_info;
    struct timeval begin, end;
    unsigned long fastestTime, timePassed;
    fastestTime = 1000000;
    char testholder[MAXIPADDRSIZE];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if((getaddrinfo(urlHostName, NULL, NULL, &IP_addr_info)) < 0) {
        return -1;
    }
    if(IP_addr_info == NULL) {
        return -1;
    }

    int sockid = socket(AF_INET, SOCK_STREAM, 0);

    //Loops thorugh the addrinfo struct
    while (IP_addr_info != NULL) {
        memset(testholder, 0, MAXIPADDRSIZE);
        inet_ntop(AF_INET, &((struct sockaddr_in *) IP_addr_info->ai_addr)->sin_addr, testholder, 400);
        if (strcmp(testholder, "0.0.0.0") == 0){
            IP_addr_info = IP_addr_info->ai_next;
            continue;
        } else {
            gettimeofday(&begin, NULL);
            connect(sockid, IP_addr_info->ai_addr, IP_addr_info->ai_addrlen);
            gettimeofday(&end, NULL);
            timePassed = end.tv_usec - begin.tv_usec;
            if (timePassed < fastestTime) {
                fastestTime = timePassed;
                inet_ntop(AF_INET, &((struct sockaddr_in *) IP_addr_info->ai_addr)->sin_addr, ipAddrHost, 400);
            }
            IP_addr_info = IP_addr_info->ai_next;
        }
    }

    freeaddrinfo(IP_addr_info);
    return 0;
}

void doHTTP(int servSocket, int clientSocket, char* ptrHOSTIP, char* ptrURL, char* ptrPATH, char* ptrPORT) {

    char request[MAXSIZEOFREQUEST];
    char response[MAXSIZEOFRESPONSE];

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    if (strlen(ptrPORT) == 0) {
        serverAddr.sin_port = htons(80);
    } else {
        short port = atoi(ptrPORT);
        serverAddr.sin_port = htons(port);
    }

    inet_pton(AF_INET, ptrHOSTIP, &serverAddr.sin_addr);

    servSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (servSocket < 0) {
        perror("servSocket");
    }

    int conn = connect(servSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    if (conn < 0) {
        perror("Connection");
    }

    //The HTTP GET request to send
    strcpy(request, "GET ");
    strcat(request, ptrPATH);
    strcat(request, " HTTP/1.1\r\nHost: ");
    strcat(request, ptrURL);
    if (strlen(ptrPORT) == 0) {
        strcat(request, "\r\n\r\n");
    } else {
        strcat(request, ":");
        strcat(request, ptrPORT);
        strcat(request, "\r\n\r\n");

    }

    write(servSocket, request, sizeof(request));

    long bytesRead = 0;
    int total = 0;

    while ((bytesRead = read(servSocket, response, sizeof(response))) > 0 ) {
        write(clientSocket, response, bytesRead);
        total += bytesRead;
    }
    if (bytesRead == -1) {
        perror("recv");
    } else if (bytesRead == 0) {
        memset(response, 0, MAXSIZEOFRESPONSE);
    }
}


int blockedSites(char* filename, char* ptrURL) {
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        char line[256];

        while(fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            if (strcmp(ptrURL, line) == 0) {
                printf("REQ: %s BLOCKED\n", ptrURL);
                return -1;
            }
            memset(line , 0, sizeof(line));
        }
    } else {
        return 0;
    }
    fclose(file);

    return 0;
}


int main(int argc, char *argv[]) {
    char ptrHOSTIP[MAXIPADDRSIZE], ptrURL[MAXSIZEOFREQUEST], ptrPATH[MAXSIZEOFREQUEST], buff[MAXSIZEOFREQUEST - 1], ptrPORT[24];
    char* filename;

    int port, clientSocket, proxSocket, bindClient;

    port = atoi(argv[1]);
    filename = argv[2];

    proxSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (proxSocket < 0) {
        perror("error with opening socket");
        exit(-1);
    }

    proxAddr.sin_family = AF_INET;
    proxAddr.sin_port = htons(port); //unsigned short integer hostshort from host byte order to network byte order
    proxAddr.sin_addr.s_addr = htonl(INADDR_ANY); //unsigned integer hostlong from host byte order ot network byte order

    //Where the connection takes place
    bindClient = bind(proxSocket, (struct sockaddr *) &proxAddr, sizeof(proxAddr));
    if (bindClient < 0) {
        perror("Error accepting and binding");
        exit(-1);
    }

    while (1) {

        listen(proxSocket, 3);
        printf("Stage 3 proxy started on port %d (by oeh3@njit.edu)\n", port);
        printf("\n");
        socklen_t clientLen = sizeof(clientAddr);
        clientSocket = accept(proxSocket, (struct sockaddr *) &clientAddr, &clientLen);
        if (clientSocket < 0) {
            perror("Error while accepting");
            exit(-1);
        }else {
            //Clears memory for every new connection
            memset(buff, 0, MAXSIZEOFREQUEST);
            memset(ptrURL, 0, MAXSIZEOFREQUEST);
            memset(ptrPATH, 0, MAXSIZEOFREQUEST);
            memset(ptrHOSTIP, 0, MAXIPADDRSIZE);
            memset(ptrPORT, 0, 24);

            int sizeRecv = recv(clientSocket, buff, MAXSIZEOFREQUEST, 0);
            if (sizeRecv < 0) {
                perror("Recv");
            }

            doParse(buff, ptrURL, ptrPATH, ptrPORT);

            if (ptrURL == NULL) {
                errorMes(400, clientSocket);
            }
            printf("REQUEST: %s\n", ptrURL);

            int dnsError = dns(ptrURL, ptrHOSTIP);
            int servSocket = socket(AF_INET, SOCK_STREAM, 0);

            if (dnsError < 0) {
                errorMes(404, clientSocket);
            }

            if (blockedSites(filename, ptrURL) < 0) {
                //SEND ERROR
                errorMes(403, clientSocket);
            } else {
                doHTTP(servSocket, clientSocket, ptrHOSTIP, ptrURL, ptrPATH, ptrPORT);
            }
            close(clientSocket);
            close(servSocket);

        }
    }

}

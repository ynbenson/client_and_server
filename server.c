/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"
#include "string.h"
#include "dirent.h"
#define S_FILECONTENT 100000
#define S_FILENAME 80
#define S_STRUCT 88
#define STORE 1
#define RETRIEVE 2
#define DELETE 3
#define LIST 4
#define QUIT 5
#define SUCCESS 0
#define ERROR -1

struct str_store {
    int type;
    unsigned int secret_key; // 4 byte (32 bit) unsigned integer => 0 ~ 4294967295
    char filename[S_FILENAME];
} s1;

struct str_retrieve {
    int response;
    unsigned int byte;
    char file_content[S_FILECONTENT];
} s2;

int main(int argc, char **argv) 
{
    int listenfd, connfd, file_byte, store_response, delete_response, fd, type, num_file, close_connection, key_from_client, list_response;
    socklen_t clientlen;
    FILE *fp;
    unsigned int key;
    struct stat info;
    struct dirent *dp;
    DIR *dir;
//    char path[80];
    
    
    // sockaddr => Structure which stores
    //          (1) Transmission Protocol
    //          (2) Address
    //          (3) Port Number
    struct sockaddr_storage clientaddr; // sockaddr_storage => 128 byte  
    
    char client_hostname[MAXLINE], client_port[MAXLINE], buf[S_FILENAME], file_content[S_FILECONTENT];

    if (argc != 3) {
	    fprintf(stderr, "usage: %s <port> <key>\n", argv[0]);
	    exit(0);
    }
    
    key = atoi(argv[2]);
    
    // Create Socket 
    listenfd = Open_listenfd(argv[1]);
    
    
    clientlen = sizeof(struct sockaddr_storage);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    while (1) {
        Rio_readn(connfd, &type, 4);
        Rio_readn(connfd, &key_from_client, 4);
        
        //printf("key = %d\n", key);
        //printf("s1.secret_key = %d\n", key_from_client);
        
        
        if(key_from_client != key){
            close_connection = 1;
        }
        else {
            close_connection = 0;
        }
        
        Rio_writen(connfd, &close_connection, 4);
        
        if(close_connection)
            break;
        
        
        //printf("type = %d\n", type);
        if(type == QUIT)
            break;
        
        switch(type){
            case STORE:
                printf("STORE\n");
                // Read struct with "type", "secret_key", and "filename"
                Rio_readn(connfd, &s1, S_STRUCT);
#ifdef DEBUG
                printf("s1.type = %d\n", s1.type);
                printf("s1.key = %u\n", s1.secret_key);
                printf("filename = %s\n", s1.filename);
#endif
                Rio_readn(connfd, &file_byte, 4);
                Rio_readn(connfd, file_content, file_byte);
#ifdef DEBUG
                printf("file_byte = %d\n", file_byte);
                printf("file_content = %s\n", file_content);
#endif
                if((fp = fopen(s1.filename, "w")) == NULL){
                    store_response = ERROR;
                    printf("Error when opening %s\n", s1.filename);
                }
                else {
                    store_response = SUCCESS;
                    
                    // Writing to a file
                    Fwrite(&file_content[0], 1, file_byte, fp);
                    fclose(fp);
                }
                    
                // Send Store Response back to Client 
                Rio_writen(connfd, &store_response, 4);

                break;

            case RETRIEVE:
                printf("RETRIEVE\n");
                // Read struct with "type", "secret_key", and "filename"
                Rio_readn(connfd, &s1, S_STRUCT);
                
                if((fd = open(s1.filename, O_RDONLY)) < 0){
                    fprintf(stderr, "Error\n");
                    s2.response = ERROR;
                }
                else {
                    if(fstat(fd, &info) != 0){
                        fprintf(stderr, "Error\n");
                        s2.response = ERROR;
                    }
                    else{
                        // read content of file into filecontent
                        Rio_readn(fd, s2.file_content, info.st_size);
                        s2.byte = info.st_size;
                        s2.response = SUCCESS;
#ifdef DEBUG
                        printf("size = %lld bytes\n", info.st_size);
                        printf("file content = %s\n", s2.file_content);
#endif
                    }
                }
                
                Rio_writen(connfd, &s2, sizeof(s2));
                
                break;
                
            case DELETE:
                printf("DELETE\n");
                //printf("delete in server\n");
                
                // Read struct with "type", "secret_key", and "filename"
                Rio_readn(connfd, &s1, S_STRUCT);
                
                if(remove(s1.filename) == 0){
                    printf("%s successfully removed\n", s1.filename);
                    delete_response = SUCCESS;
                }
                else {
                    printf("Error removing %s\n", s1.filename);
                    delete_response = ERROR;
                }
                
                Rio_writen(connfd, &delete_response, 4);
                break;
                
            case LIST:
                printf("LIST\n");
                Rio_readn(connfd, &s1, 8);
                
                //printf("s1.type = %d\n", s1.type);
                //printf("s1.key = %d\n", s1.secret_key);
                       
                if((dir = opendir("."))== NULL){
                    printf("Error\n");
                    list_response = ERROR;
                } else {
                    for(dp=readdir(dir), num_file = 0; dp != NULL; dp = readdir(dir)){
                        //printf("%s\n", dp->d_name);
                        num_file++;
                    }
                    
                    
                    
                    //printf("num_file = %d\n", num_file);
                    Rio_writen(connfd, &num_file, 4);
                    
                    closedir(dir);
                    dir = opendir(".");
                    
                    for(dp=readdir(dir), num_file = 0; dp != NULL; dp = readdir(dir)){
                        strcpy(buf, dp->d_name);
                        Rio_writen(connfd, buf, S_FILENAME);
                    }
                    list_response = SUCCESS;
                    
                    closedir(dir);
                }
                
                Rio_writen(connfd, &list_response, 4);
                
                break;
                
            case ERROR:
                printf("Invalid command\n");
                break;

            default:
                // printf("default\n");
                break;
        }
    
    }
    Close(connfd);

    exit(0);
}
/* $end echoserverimain */

/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"
#define S_FILECONTENT 100000
#define S_STRUCT 88
#define S_FILENAME 80
#define S_ARGV 10
#define STORE 1
#define RETRIEVE 2
#define DELETE 3
#define LIST 4
#define QUIT 5
#define SUCCESS 0
#define ERROR -1

struct str_store {
    int type;
    // 4 byte (32 bit) unsigned integer => 0 ~ 4294967295
    unsigned int secret_key; 
    char filename[S_FILENAME]; 
} s1;

struct str_retrieve {
    int response;
    unsigned int byte;
    char file_content[S_FILECONTENT];
} s2;

void separate_into_tokens(char* buf, char** my_argv, int* my_argc){
    buf[strlen(buf)-1]='\0';

    my_argv[0] = strtok(buf, " ");
    
    for(*my_argc=1; ; ++(*my_argc)){
        my_argv[*my_argc] = strtok(NULL, " ");
        if(my_argv[*my_argc]==NULL)
            break;
    }
}

int main(int argc, char **argv) 
{

    int clientfd, my_argc, fd, file_byte, store_response, delete_response, num_file, i, close_connection, list_response;
    unsigned int key, struct_size;
    char *host, *port, buf[MAXLINE], filename[S_FILENAME], filecontent[S_FILECONTENT], *my_argv[S_ARGV];
    FILE *fp;
    struct stat info;

    rio_t rio;

    if (argc != 4) {
	    fprintf(stderr, "usage: %s <host> <port> <key>\n", argv[0]);
	    exit(0);
    }
    host = argv[1];
    port = argv[2];
    key = atoi(argv[3]); 
    
    clientfd = Open_clientfd(host, port);

    // Rio_readinitb(&rio, clientfd);

    while (1) {
        printf(">");
        if(Fgets(buf, MAXLINE, stdin) == NULL) break;

        separate_into_tokens(buf, my_argv, &my_argc);
        
        //printf("my_argc = %d\n", my_argc);
        
        
        
        s1.secret_key = key;

        if(strcmp(my_argv[0], "cput") == 0){
            if(my_argc == 2)
                s1.type = STORE;
            else
                s1.type = ERROR;
        }
        else if(strcmp(my_argv[0],"cget") == 0){
            if(my_argc == 2)
                s1.type = RETRIEVE;
            else
                s1.type = ERROR;
        }
        else if(strcmp(my_argv[0],"cdelete") == 0){
            if(my_argc == 2)
                s1.type = DELETE;
            else
                s1.type = ERROR;
        }
        else if(strcmp(my_argv[0],"clist") == 0){
            if(my_argc == 1)
                s1.type = LIST;
            else
                s1.type = ERROR;
        }
        else if(strcmp(my_argv[0],"quit") == 0){
            if(my_argc == 1)
                s1.type = QUIT;
            else
                s1.type = ERROR;
        }
        else // none of the choice
            s1.type = ERROR;
        
#ifdef DEBUG
        if(my_argc > 1){
            printf("sizeof(s1) = %lu\n", sizeof(s1));
            struct_size = sizeof(s1);
            printf("s1.type = %d\n", s1.type);

            if(my_argc == 2){
                printf("s1.key = %d\n", s1.secret_key);
                printf("filename = \"%s\"\n", my_argv[1]);
            }
        }
#endif
        if(my_argc == 2)
            strcpy(s1.filename, my_argv[1]);

        
        Rio_writen(clientfd, &s1.type, 4);
        Rio_writen(clientfd, &s1.secret_key, 4);
        Rio_readn(clientfd, &close_connection, 4);
        
        if(close_connection){
            printf("Secret key doesn't match. Closing connection\n");
            break;
        }
        
        
        if(s1.type == QUIT)
            break;
        
        switch (s1.type) {
            case STORE:
                Rio_writen(clientfd, &s1, sizeof(s1));
                if(my_argc == 2){
                    if((fd = open(s1.filename, O_RDONLY)) < 0){
                        fprintf(stderr, "Error\n");
                    }
                    else {
                        if(fstat(fd, &info) != 0){
                            fprintf(stderr, "Error\n");
                        }
                        else{
                            // read content of file into filecontent
                            Rio_readn(fd, filecontent, info.st_size);
#ifdef DEBUG
                            printf("size = %lld bytes\n", info.st_size);
                            printf("file content = %s\n", filecontent);
#endif
                            Rio_writen(clientfd, &info.st_size, 4);
                            Rio_writen(clientfd, filecontent, info.st_size);
                            
                            // receive Store Response
                            Rio_readn(clientfd, &store_response, 4);
                            printf("STORE RESPONSE : %d\n", store_response);
                            
                        }
                    }
                }
                else {
                    printf("Usage: <cput> <filename> \n");
                }
                break;
                
            case RETRIEVE:
                if(my_argc == 2){
                    Rio_writen(clientfd, &s1, sizeof(s1));
                    Rio_readn(clientfd, &s2, sizeof(s2));
                    
                    if(s2.response == SUCCESS){
                        if((fp = fopen(s1.filename, "w")) == NULL){
                            printf("Error when opening %s\n", s1.filename);
                        }
                        else {
    #ifdef DEBUG
                            printf("size = %u bytes\n", s2.byte);
                            printf("file content = %s\n", s2.file_content);
    #endif
                            Fwrite(&s2.file_content[0], 1, s2.byte, fp);
                            fclose(fp);
                        }
                    }
                    printf("RETRIEVE RESPONSE : %d\n", s2.response);
                }
                else {
                    printf("Usage: <cget> <filename> \n");
                }
                
                break;
                
            case DELETE:
                Rio_writen(clientfd, &s1, sizeof(s1));
                //printf("delete\n");
                Rio_readn(clientfd, &delete_response, 4);
                printf("DELETE RESPONSE : %d\n", delete_response);
                
                break;
                
            case LIST:
                //printf("list\n");
                //printf("ahoho\n");
                Rio_writen(clientfd, &s1, 8);
                Rio_readn(clientfd, &num_file, 4);
                
                //printf("num_file = %d\n", num_file);
                
                for(i=0; i<num_file; ++i){
                    Rio_readn(clientfd, filename, S_FILENAME);
                    printf("%s\n", filename);
                }
                
                Rio_readn(clientfd, &list_response, 4);
                printf("LIST RESPONSE : %d\n", list_response);

                break;
                
            default:
                printf("Invalid command\n");
                break;
        }
    }
    
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */

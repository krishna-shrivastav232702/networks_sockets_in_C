#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<sys/wait.h>
#include<signal.h>
#include<netinet/in.h>
#include<arpa/inet.h>


#define PORT "3490"
#define BACKLOG 10

void sigchld_handler(int s){ //single child handler
    (void)s;
    int saved_errno = errno;
    while(waitpid(-1,NULL,WNOHANG)>0);
    errno = saved_errno;
}

void* get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(){
    int sockfd, new_fd;
    struct addrinfo hints,*servinfo,*p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL,PORT,&hints,&servinfo))!=0){
        fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
        return 1;
    }

    //loop through all the results and bind to the first we can
    //Iterates over the linked list of address results returned by getaddrinfo()
    for(p = servinfo; p!=NULL;p=p->ai_next){
        if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1){
            perror("Server:socket");
            continue;
        }
        if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1){ //This tells the kernel to reuse the port even if it’s in a TIME_WAIT state.
            perror("setsockopt");
            exit(1);
        }
        if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1){
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break; // breaks when first socket connection is done
    }
    freeaddrinfo(servinfo);
    if(p==NULL){
        fprintf(stderr,"server: failed to bind \n");
        exit(1);
    }
    if(listen(sockfd,BACKLOG)==-1){
        perror("listen");
        exit(1);
    }
    // sa coming from struct sigaction // which is used for signal handling
    sa.sa_handler = sigchld_handler; // reap all dead processes // Tell OS: "This is my signal handler"
    sigemptyset(&sa.sa_mask);  //Don’t block any other signals inside the handler
    sa.sa_flags = SA_RESTART; // If a syscall is interrupted, restart it (ex: accept())
    if(sigaction(SIGCHLD,&sa,NULL)==-1){ //Tell the OS to use this for SIGCHLD
        perror("sigaction");
        exit(1);
    }

    printf("sever: waiting for connections");
    //continuously waits for new client connections and handles them
    while(1){
        sin_size = sizeof their_addr; //their_addr will hold the incoming client’s address 
        new_fd = accept(sockfd,(struct sockaddr *)&their_addr, &sin_size);
        if(new_fd == -1){
            perror("accept");
            continue;
        }
        //convert binary IP to human-readable string
        //get_in_addr() = helper that extracts the correct part of the sockaddr struct (IPv4 or IPv6).
        //s = char array where the readable IP is stored.
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s,sizeof s);
        printf("server got connection from %s \n",s);
        if(!fork()){
            close(sockfd);
            if(send(new_fd,"Hello Krishna",13,0)==-1){
                perror("send");
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    return 0;
}


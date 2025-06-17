#include<stdio.h>
#include<poll.h>

int main(){
    struct pollfd pfds[1];
    pfds[0].fd = 0;
    pfds[0].events = POLLIN; //tell me when ready to read

    printf("Hit Return or wait 2.5 seconds for timeout \n");

    int num_events = poll(pfds,1,2500);

    if(num_events == 0){
        printf("Poll timed out \n");
    }else{
        int pollin_happened = pfds[0].revents & POLLIN; //It checks if the POLLIN bit is set in the revents field — i.e., did we get a read event?
        if(pollin_happened){
            printf("file descriptor %d is ready to read \n",pfds[0].fd);
        }else{
            printf("Unexpected event occured : %d \n",pfds[0].revents);
        }
    }
}
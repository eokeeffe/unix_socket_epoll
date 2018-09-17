// C headers
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
// C System headers
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/epoll.h>
// C++ headers
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <csignal>
#include <ctime>
#include <chrono>
#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cassert>

// EPOLL non blocking
#define EPOLL_RUN_TIMEOUT 0
// EPOLL
#define EPOLL_EVENTS 10
#define MAX_EVENTS 64

char server_socket_name[] = "server.s";

int setNonblocking(int fd);
std::string handle_io_on_socket(int fd);
int setupSocket(char *socket_name);
void closeSocket(char *socket_name);
void signal_callback_handler(int signum);


std::string getResponse(int epfd, int fd){
    char buffer[8];
    
    ssize_t bytes = read(fd, &buffer, 8);
    
    if(bytes > 0){
        return std::string(buffer);
    }else{
        //epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        return "";
    }
}

int setNonblocking(int fd)
{
    /*----------------------------------------------------------------------
     Portable function to set a socket into nonblocking mode.
     Calling this on a socket causes all future read() and write() calls on
     that socket to do only as much as they can immediately, and return
     without waiting.
     If no data can be read or written, they return -1 and set errno
     to EAGAIN (or EWOULDBLOCK).
     Thanks to Bjorn Reese for this code.
    ----------------------------------------------------------------------*/

    int flags;

    /* If they have O_NONBLOCK, use the Posix way to do it */
    #if defined(O_NONBLOCK)
        /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
        if (-1 == (flags = fcntl(fd, F_GETFL, 0))){
            flags = 0;
        }
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    #else
        /* Otherwise, use the old way of doing it */
        flags = 1;
        return ioctl(fd, FIOBIO, &flags);
    #endif
}


int setupSocket(char *socket_name){

    int s,len;
    struct sockaddr_un sock_addr;

    if ((s = socket(AF_UNIX, SOCK_STREAM , 0)) < 0) {
        perror("server: socket");
        exit(1);
    }

    sock_addr.sun_family = AF_UNIX;
    strcpy(sock_addr.sun_path, socket_name);

    unlink(socket_name);
    remove(socket_name);
    len = sizeof(sock_addr.sun_family) + strlen(sock_addr.sun_path);

    int opt_val = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    if (bind(s,(struct sockaddr *) &sock_addr, len) < 0) {
        std::cerr << "server: bind" << std::endl;
        exit(1);
    }

    if(setNonblocking(s) < 0){
        std::cerr << "Couldn't set server non blocking" << std::endl;
        exit(1);
    }

    if (listen(s, 10) < 0) {
        std::cerr << "server: listen" << std::endl;
        exit(1);
    }

    return s;
}

void closeSocket(char *socket_name){
    unlink(socket_name);
    remove(socket_name);
}

void signal_callback_handler(int signum)
{
   printf("Caught signal %d\n",signum);
   // Cleanup and close up stuff here

   closeSocket(server_socket_name);

   // Terminate program
   exit(signum);
}

int main(int argc,char *argv[]){

    int serial_socket = setupSocket(server_socket_name);

    int epfd = epoll_create(EPOLL_EVENTS);

    if(epfd == -1){
        std::cerr << "Coudln't epoll_create'" << std::endl;
    }

    struct sockaddr_un peer_addr;
    socklen_t addrlen= sizeof(peer_addr);

    memset(&peer_addr, 0, sizeof(struct sockaddr_un));

    struct epoll_event event;
    struct epoll_event event_input;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET ;  // EPOLLIN==read, EPOLLOUT==write
    event.data.fd = serial_socket;
    int res = epoll_ctl(epfd, EPOLL_CTL_ADD, serial_socket, &event);
    if(res == -1){
        std::cerr << "Could't setup sockets with EPOLL" << std::endl;
    }

    struct epoll_event *events ;
    events = (struct epoll_event*) malloc(sizeof (struct epoll_event) * MAX_EVENTS);
    if (!events) {
        perror ("malloc");
        return 0;
    }

    int connected_sock = 0;
    struct sockaddr_un client_address;
    
    signal(SIGINT, signal_callback_handler);
    
    while(1){
    
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, EPOLL_RUN_TIMEOUT);
        for(int i = 0; i < nfds; i++){
                
                if(events[i].events & EPOLLIN && serial_socket == events[i].data.fd){
                    
                    int client_len = sizeof(client_address);
    
                    connected_sock = accept(serial_socket, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
                    if(connected_sock < 0){
                        std::cerr << "Couldn't connect socket" << std::endl;
                        continue;
                    }
                    setNonblocking(connected_sock);
                    
                    std::string response = getResponse(epfd,connected_sock);
                    std::cout << "Response:" << response << std::endl;
                    
                    close(connected_sock);
                }
       }
    }

    return 0;
}

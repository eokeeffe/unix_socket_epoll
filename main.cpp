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

void writeSocket(char socket_name[9],std::string input){
    struct sockaddr_un server;
    int sock = socket(AF_UNIX, SOCK_STREAM , 0);//| O_NONBLOCK
    if (sock < 0) {
        perror("opening serial input stream socket");
        exit(1);
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, socket_name);
    if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
        close(sock);
        //std::cerr<<"connecting stream socket"<<std::endl;
    }
    if (write(sock, input.c_str(), input.length()) < 0){
        //std::cerr<<"writing on stream socket"<<std::endl;
    }
    close(sock);
}

long getTime(std::chrono::steady_clock::time_point begin,
        std::chrono::steady_clock::time_point end, char type){

    long elapsed = 0;

    switch(type){
        case 0:{//nano second
            elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            break;
        }
        case 1:{//micro second
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
            break;
        }
        case 2:{//milli second
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            break;
        }
        case 3:{//seconds
            elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
            break;
        }
        case 4:{//minutes
            elapsed = std::chrono::duration_cast<std::chrono::minutes>(end - begin).count();
            break;
        }
        case 5:{//hours
            elapsed = std::chrono::duration_cast<std::chrono::hours>(end - begin).count();
            break;
        }
        default:{
            elapsed = 0;
            break;
        }
    }

    return elapsed;
}

int main(int argc,char *argv[]){
    
    char socket_name[9] = "server.s";
    
    auto start_of_program = std::chrono::steady_clock::now();
    auto end_of_program = std::chrono::steady_clock::now();
    
    long counter = 0;
    
    while(1){
		if(getTime(start_of_program,end_of_program,2) > 1000){
		    std::string output = "123456789";
		    writeSocket(socket_name, output);
		    start_of_program = std::chrono::steady_clock::now();
		}
		end_of_program = std::chrono::steady_clock::now();
		counter ++;
    }
    return 0;
}

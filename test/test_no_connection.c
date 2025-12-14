#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// #include "Protocol/protocol.h"
// #include "Server/server.h"
#include "../include/Protocol/protocol.h"
#include "../include/Server/server.h"
#include "../include/User/user.h"

int main() {
     user_t user;
     user_init(&user);
     user_run(&user, "10.254.141.231", 17001);
     return 0;
}

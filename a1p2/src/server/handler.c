#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/utsname.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include "../protocol.h"
#include "server.h"

void* handle_client(threadcommon* common, void* arg) {
    cnode* clientptr = (cnode*)arg;
    clientdata* client = &clientptr->data;
    int socket = client->socket;


    char hin,hout;
    net_buffer buffer_send = buffer_create();
    net_buffer buffer_recv = buffer_create();
    char string_send[INPUT_MAX], string_recv[INPUT_MAX];
    int read_size, infractions = 0;

    //printf("client thread is active, socket %i, name %s\n",socket,client->name);

    //pthread_mutex_lock(&common->lock);
    /*if(common->state == GAME_STATE_WAIT){
        buffer_seek(&buffer_send,0);
        hout = HEADER_MOVE;
        buffer_write(&buffer_send,&hout,sizeof(char));
        buffer_write(&buffer_send,&common->val,sizeof(char));
        send(socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
        common->state = GAME_STATE_GO;
    }*/

    while(common->all_terminate | client->terminate == 0){
        //if(client_find(common,clientptr) == 0) break;
        buffer_seek(&buffer_recv,0);
        buffer_seek(&buffer_send,0);
        memset(string_recv,'\0',sizeof(char)*INPUT_MAX);
        memset(string_send,'\0',sizeof(char)*INPUT_MAX);
        int size_in = recv(socket, buffer_recv.buffer, PACKET_MAX, 0);
        //if(common->all_terminate | client->terminate != 0) break;
        if(size_in < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
            else break;
            //perror("recv failed");
            //break;
        }else if(size_in == 0){
            //printf("disconnect\n");
            break;
        }
        buffer_recv.buffer[size_in] = '\0';
        hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        // PACKET STREAMING HOLY MOLY
        int pktc = 0;
        while(hin != '\0'){
            pktc++;
            switch(hin){
                /*case HEADER_PING:
                    client->ping = NETWORK_TIMEOUT_PING;
                    //printf("ping %s\n",client->name);
                break;*/

                case HEADER_INFO:
                    buffer_read_string(&buffer_recv,string_recv);
                    strncpy(client->name,string_recv,sizeof(client->name));
                    //printf("client '%i' is named '%s'\n",socket,client->name);

                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s joined the game.",client->name);
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);
                break;

                case HEADER_TEXT:
                    //printf("text checker inator %s\n",client->name);
                    buffer_read_string(&buffer_recv,string_recv);
                    //printf("client '%s' says '%s'\n",client->name,string_recv);

                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s: %s",client->name,string_recv);

                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);

                break;

                case HEADER_MOVE:
                    char move = *(char*)buffer_read(&buffer_recv,sizeof(char));

                    //TODO: sanitize this move ^^^

                    //printf("client '%s' moves '%i'\n",client->name,move);

                    if(move == (char)fmin(fmax(move,1),9)){
                        client_rotate(common/*,common->sockets*/);
                        common->val -= move;
                        infractions = 0;
                    }else{
                        if(infractions >= GAME_INFRACTION_LIMIT){
                            client->terminate = 1;
                            continue;
                        }
                        infractions++;
                    }

                    if(common->val <= 0){
                        char* msg[] = {"you win :)","you lose :("};
                        snprintf(string_send,INPUT_MAX*sizeof(char),"%s won the game.",client->name);

                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_ALL);

                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[0]);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_TO);

                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[1]);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);

                        common->state = GAME_STATE_WAIT;
                        common->val = common->valdef;
                    }else{
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_MOVE;
                        char vout = common->val;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write(&buffer_send,&vout,sizeof(char));
                        send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);


                        snprintf(string_send,INPUT_MAX*sizeof(char),"%s is taking their turn.",common->sockets->data.name);
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),common->sockets->data.socket,NETWORK_TARGET_EXCEPT);
                    }
                break;

                case HEADER_END:
                    printf("%s disconnected\n",client->name);
                    client->terminate = 1;

                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s left the game.",client->name);
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);
                break;

                default:
                    printf("ERROR malformed packet or bad header from %s. sending them to the shadow realm\n",client->name);
                    network_disconnect(clientptr);
                    client->terminate = 1;
                    pktc--;
                break;
            }
            hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        }
        //pthread_mutex_unlock(&common->lock);
    }

    close(socket);
    client_remove(common,clientptr);
    return NULL;
}

void* network_thread_actor(void* arg){
    threadcommon* common = (threadcommon*)arg;
    net_buffer buffer_send = buffer_create();
    char hout;
    int time = clock();
    char* msg[] = {"you win, because everyone else is gone","timed out on your turn","timed out from no ping response"};
    char string_send[INPUT_MAX];

    int turntime = clock();//, turn = 0;
    cnode* turn = NULL;
    //printf("starting thread actor\n");
    while(common->all_terminate == 0){
        if(clock()-time > CLOCKS_PER_SEC && common->sockets != NULL){
            pthread_mutex_lock(&common->lock);
            time = clock();
            //printf("...");
            int clientcount = client_count(common);
            if(common->state == GAME_STATE_GO){
                if(clientcount >= GAME_MIN_PLAYERS){
                    if(turn != common->sockets){
                        turn = common->sockets;
                        turntime = clock();
                    }else if(clock()-turntime > NETWORK_TIMEOUT_WAIT*CLOCKS_PER_SEC){
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[1]);
                        send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        snprintf(string_send,INPUT_MAX*sizeof(char),"%s was removed from the game.",common->sockets->data.name);
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),common->sockets->data.socket,NETWORK_TARGET_EXCEPT);

                        network_disconnect(common->sockets);
                    }

                    // remind player that its their turn, in case client loses the packet even though its tcp and that wont happen
                    buffer_seek(&buffer_send,0);
                    hout = HEADER_MOVE;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    //char val = common->val;
                    buffer_write(&buffer_send,&common->val,sizeof(char));
                    send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
                }else{
                    if(clientcount == 1){
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[0]);
                        send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
                    }
                    common->val = common->valdef;
                    common->state = GAME_STATE_WAIT;
                }
            }else if(common->state == GAME_STATE_WAIT && clientcount >= common->min){
                common->state = GAME_STATE_GO;
                printf("have enough players, starting!\n");

                buffer_seek(&buffer_send,0);
                hout = HEADER_TEXT;
                buffer_write(&buffer_send,&hout,sizeof(char));
                snprintf(string_send,INPUT_MAX*sizeof(char),"Game starting with %i players. Starting value is %i",clientcount,common->val);
                buffer_write_string(&buffer_send,string_send);
                send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),-1,NETWORK_TARGET_ALL);
            }else if(common->state == GAME_STATE_WAIT){
                turn = NULL;
                turntime = clock();
            }
            pthread_mutex_unlock(&common->lock);
        }
    }
}

void* network_thread_listener(void* arg){
    threadcommon* common = (threadcommon*)arg;
    struct sockaddr_in client;
    int clientsock;
    //printf("starting thread listener\n");
    while (common->all_terminate == 0) {
        int clientlen = sizeof(client);
        clientsock = accept(common->serversock, (struct sockaddr*)&client, &clientlen);
        if (clientsock < 0) {
            perror("ERROR accept failed");
            continue;
        }

        /*int flags = fcntl(clientsock, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl F_GETFL");
            continue;
        }

        // Set the file descriptor flags to non-blocking mode
        flags |= O_NONBLOCK;
        if (fcntl(clientsock, F_SETFL, flags) == -1) {
            perror("fcntl F_SETFL O_NONBLOCK");
            continue;
        }*/
        //clientdata data;
        //data.socket = clientsock;
        //clientdata* data = (clientdata*)malloc(sizeof(clientdata));
        //data->socket = clientsock;

        //client.name = NULL;
        //common->sockets[common->socketcount++] = data;
        //printf("tryna create a linked list over eere\n");
        //pthread_mutex_lock(&common->lock);
        if(common->state == GAME_STATE_WAIT){
            char* user = "user";
            client_insert(common);
            strncpy(common->sockets->data.name,user,sizeof(common->sockets->data.name));
            common->sockets->data.socket = clientsock;
            common->sockets->data.terminate = 0;
            common->sockets->data.ping = NETWORK_TIMEOUT_PING;
            //if(common->sockets->next != NULL) printf("this:'%i' next:'%i'",common->sockets->data.socket,common->sockets->next->data.socket);
            //printf("joined game\n");

            /*net_buffer buffer_send = buffer_create();
            char hout = HEADER_INFO, state = common->state;
            buffer_seek(&buffer_send,0);
            hout = HEADER_TEXT, msg[INPUT_MAX];

            snprintf(msg,INPUT_MAX*sizeof(char),"%s won the game.",client->name);

            buffer_write(&buffer_send,&hout,sizeof(char));*/

            // Enqueue the client socket
            enqueue(common->sockets,common);
        }else{
            net_buffer buffer_send = buffer_create();
            char hout, *strout = "kicked because the session is currently active";
            buffer_seek(&buffer_send,0);
            hout = HEADER_TEXT;
            buffer_write(&buffer_send,&hout,sizeof(char));
            buffer_write_string(&buffer_send,strout);
            hout = HEADER_END;
            buffer_write(&buffer_send,&hout,sizeof(char));
            send(clientsock,buffer_send.buffer,buffer_tell(&buffer_send),0);

            close(clientsock);
        }
        //memcpy(common->sockets.data,&data,sizeof(clientdata));
        //common.sockets[common.socketcount++] = clientsock;

        //pthread_mutex_unlock(&common->lock);
    }
    close(common->serversock);
}
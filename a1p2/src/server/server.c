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
#include "protocol.h"

#define PORT 8080
#define BACKLOG 10
#define BUF_SIZE 1024
#define MAXLINE 30

#define THREAD_POOL_SIZE 5

typedef struct cnode {
    clientdata data;
    struct cnode *next, *last;
} cnode;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;

    struct sockaddr_in server;
    int serversock;

    cnode* task_queue[BACKLOG];
    int task_count;
    int all_terminate;

    int socketcount;
    //clientdata sockets[16];
    cnode* sockets;

    int turn,val,state;
} threadcommon;

cnode* client_insert(cnode* root, clientdata* data){
    cnode* c = (cnode*)malloc(sizeof(cnode));
    c->data = *data;
    c->next = root;
    c->last = NULL;
    return c;
}

void client_remove(cnode* node){
    if(node->last != NULL) node->last->next = node->next;
    if(node->next != NULL) node->next->last = node->last;
    free(node);
}

void client_rotate(cnode* root){
    root->next->last = root->last;
    cnode* n = root->next;
    while(n->next != NULL) n = n->next;
    n->next = root;
    root->last = n;
}   

void* handle_client(threadcommon* common, void* arg);

void enqueue(cnode* cptr, threadcommon* common) {
    pthread_mutex_lock(&common->lock);
    common->task_queue[common->task_count++] = cptr;
    pthread_cond_signal(&common->cond);
    pthread_mutex_unlock(&common->lock);
}

cnode* dequeue(threadcommon* common) {
    pthread_mutex_lock(&common->lock);
    while (common->task_count == 0) {
        pthread_cond_wait(&common->cond, &common->lock);
    }
    cnode* cptr = common->task_queue[--common->task_count];
    pthread_mutex_unlock(&common->lock);
    return cptr;
}

void* thread_worker(void* arg) {
    threadcommon *common = (threadcommon*)arg; // READ ONLY WITHOUT MUTEX LOCK
    //threadprivate *private;
    while (1) {
        cnode* cptr = dequeue(common);
        //printf("dequeue socket is::: %i",cptr->data.socket);
        handle_client(common, (void*)&cptr);
    }
    return NULL;
}

int send_all(threadcommon* common, void* buffer, int size, int target, int rule){
    /*int res = PACKET_MAX;
    for(int i = 0; i < common->socketcount; i++){
        int socket = common->sockets[i].socket, is_in_target = 0;
        if(socket == target && rule == NETWORK_TARGET_TO) is_in_target = 1;
        if(socket != target && rule == NETWORK_TARGET_EXCEPT) is_in_target = 1;
        if(rule == NETWORK_TARGET_ALL) is_in_target = 1;

        if(is_in_target == 1){
            //printf("send packet from %i to socket[%i]=%i : %s\n",socket,i,target,(char*)buffer);
            int r = send(socket,buffer,size,0);
            if(r < res) res = r;
        }
    }
    return res;*/

    cnode *node = common->sockets;
    while(node->next != NULL){
        int is_in_target = 0, socket = node->data.socket;
        if(rule == NETWORK_TARGET_ALL) is_in_target = 1;
        else if(socket == target && rule == NETWORK_TARGET_TO) is_in_target = 1;
        else if(socket != target && rule == NETWORK_TARGET_EXCEPT) is_in_target = 1;

        if(is_in_target == 1) send(socket,buffer,size,0);
        node = node->next;
    }
    return 0;
}

void* handle_client(threadcommon* common, void* arg) {
    //int id = *((int*)arg);
    cnode* clientptr = (cnode*)arg;
    //clientdata* client = &common->sockets[id];
    clientdata* client = &clientptr->data;
    int socket = client->socket;


    //char name[32] = client->name;
    //char buffer_send[PACKET_MAX];
    //char buffer_recv[PACKET_MAX];
    char hin,hout;
    net_buffer buffer_send = buffer_create();
    net_buffer buffer_recv = buffer_create();
    char string_send[INPUT_MAX], string_recv[INPUT_MAX];
    int read_size, thread_terminate = 0, infractions = 0;

    printf("client thread is active, socket %i, name %s\n",socket,client->name);

    pthread_mutex_lock(&common->lock);
    if(common->state == GAME_STATE_WAIT){
        //packet_move packet_send;
        //packet_send.header = HEADER_MOVE;
        //packet_send.move = common->val;
        //buffer_send[0] = (char)HEADER_MOVE;
        //buffer_send[1] = (char)common->val;
        //memcpy(buffer_send,(void*)&packet_send,sizeof(packet_send));
        //network_send_raw(common,buffer_send,socket,NETWORK_TARGET_TO);
        buffer_seek(&buffer_send,0);
        hout = HEADER_MOVE;
        buffer_write(&buffer_send,&hout,sizeof(char));
        buffer_write(&buffer_send,&common->val,sizeof(char));
        send(socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
        common->state = GAME_STATE_GO;
    }

    while(common->all_terminate | thread_terminate == 0){
        buffer_seek(&buffer_recv,0);
        buffer_seek(&buffer_send,0);
        memset(string_recv,'\0',sizeof(char)*INPUT_MAX);
        memset(string_send,'\0',sizeof(char)*INPUT_MAX);
        int res = recv(socket, buffer_recv.buffer, PACKET_MAX, 0);
        buffer_recv.buffer[res] = '\0';
        if(res < 0){
            perror("recv failed");
            break;
        }else if(res == 0){
            printf("disconnect\n");
            break;
        }
        //buffer_recv[res-sizeof(char)] = '\0';
        //printf("recv something");
        hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        // PACKET STREAMING HOLY MOLY
        int pktc = 0;
        while(hin != '\0'/* && hin >= HEADER_MOVE && hin <= HEADER_INFO*/){
            pktc++;
            //printf(" '%i' : ",(int)hin);
            //printf("whole buffer : %s\n",buffer_recv.buffer);
            switch(hin){
                case HEADER_INFO:
                    buffer_read_string(&buffer_recv,string_recv);
                    strncpy(client->name,string_recv,sizeof(client->name)+1);
                    printf("client '%i' is named '%s'\n",socket,client->name);
                break;

                case HEADER_TEXT:
                    //printf("text checker inator %s\n",client->name);
                    buffer_read_string(&buffer_recv,string_recv);
                    printf("client '%s' says '%s'\n",client->name,string_recv);

                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s: %s",client->name,string_recv);

                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);

                break;

                case HEADER_MOVE:
                    int move = *(char*)buffer_read(&buffer_recv,sizeof(char));
                    common->val -= move;
                    client_rotate(common->sockets);
                break;

                case HEADER_END:
                    printf("%s disconnected\n",client->name);
                    thread_terminate = 1;
                break;

                default:
                    printf("malformed packet or bad header from %s : %i\n",client->name,pktc);
                    network_disconnect(socket);
                    thread_terminate = 1;
                    pktc--;
                break;
            }
            hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        }
        //printf("finished packet handling, %i valid packets in stream\n",pktc);
        pthread_mutex_unlock(&common->lock);

        /*if(header == HEADER_INFO){
            char* txt = (char*)(buffer_recv+sizeof(char));
            printf("%i says it is named '%s'\n",socket,txt);
            //strcpy(common->sockets[get_client_from_socket(common,socket)].name,txt);
            strcpy(client->name,txt);
        }else if(header == HEADER_TEXT){
            int t = snprintf(out,sizeof(out),"%s: %s",client->name,buffer_recv+sizeof(char));
            //char* name = common->sockets[get_client_from_socket(common,socket)].name;
            //if(name != NULL) strcat(txt,name);
            
            //printf("%s: %s\n",client.name,txt);
            printf("%s\n",out);
            buffer_send[0] = HEADER_TEXT;
            memcpy(buffer_send+sizeof(char), out, strlen(out)+1);
            send_all(common,buffer_send,socket,NETWORK_TARGET_EXCEPT);
        }else if(header == HEADER_MOVE){
            int move = (int)buffer_recv[1];
            int err = -1;
            if(move != (int)fmax(fmin(move,9),1)) err = GAME_ERROR_OOB;
            if(common->turn != id) err = GAME_ERROR_SEQ;
            if(err != -1){
                printf("client bad move %i\n",move);
                infractions++;
                //network_disconnect(socket);
                snprintf(out,sizeof(out),"[server] ERROR : %s '%i'",err_game[err],move);
                buffer_send[0] = HEADER_TEXT;
                memcpy(buffer_send+sizeof(char), out, strlen(out)+1);
                send(socket,buffer_send,PACKET_MAX,0);
                if(infractions >= GAME_INFRACTION_LIMIT){
                    network_disconnect(socket);
                    break;
                }
                if(err == GAME_ERROR_OOB){
                    buffer_send[0] = HEADER_MOVE;
                    buffer_send[1] = common->val;
                    send(socket,buffer_send,PACKET_MAX,0);
                    printf("i told the client  '%i / %i' '%s' they can try again\n",client->socket,socket,client->name);
                }
                continue;
            }
            infractions = 0;
            pthread_mutex_lock(&common->lock);
            //packet_move* packet_recv = (packet_move*)buffer_recv;
            //common->val -= packet_recv->move;
            common->val -= buffer_recv[1];
            printf("state value now %i\n",common->val);

            if(common->val > 0){
                buffer_send[0] = HEADER_MOVE;
                buffer_send[1] = common->val;
                send(common->sockets[(++common->turn)%common->socketcount].socket,buffer_send,PACKET_MAX,0);
            }else{
                char* txt;
                txt = "you win :)";
                buffer_send[0] = (char)HEADER_TEXT;
                memcpy(buffer_send+sizeof(uint8_t), txt, strlen(txt)+1);
                send(socket,buffer_send,PACKET_MAX,0);
                memset(buffer_send, '\0', sizeof(buffer_send));

                txt = "you lose :(";
                buffer_send[0] = HEADER_TEXT;
                memcpy(buffer_send+sizeof(uint8_t), txt, strlen(txt)+1);
                send_all(common,buffer_send,socket,NETWORK_TARGET_EXCEPT);
                memset(buffer_send, '\0', sizeof(buffer_send));

                buffer_send[0] = HEADER_END;
                send_all(common,buffer_send,socket,NETWORK_TARGET_ALL);

                //common->all_terminate = 1;
            }
            pthread_mutex_unlock(&common->lock);
            //network_send_raw(common,buffer_send,socket,NETWORK_TARGET_TO);
        }else if(header == HEADER_END){
            printf("client say byebye\n");
            break;
        }else{
            //printf("client bad header : [%s]\n",buffer_recv);
            //network_disconnect(socket);
            //break;
        }*/
    }

    printf("terminating thread process\n");

    if (read_size == 0) {
        printf("Client disconnected\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(socket);
    //common->sockets[id] = (clientdata*)NULL;
    //common->socketcount--;
    client_remove(clientptr);
    return NULL;
}

void* network_thread_actor(void* arg){
    threadcommon* common = (threadcommon*)arg;
    net_buffer buffer_send = buffer_create();
    char hout;
    int time = clock();
    char* msg = "server: ping!";

    int turntime = clock(), turn = 0;
    while(common->all_terminate == 0){
        if(clock()-time > 5*CLOCKS_PER_SEC){
            pthread_mutex_lock(&common->lock);
            int res; 
            time = clock();
            buffer_seek(&buffer_send,0);
            hout = HEADER_TEXT;
            buffer_write(&buffer_send,&hout,sizeof(char));
            buffer_write_string(&buffer_send,msg);
            res = send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),-1,NETWORK_TARGET_ALL);
            //printf("res on ping '%i'",res);


            buffer_seek(&buffer_send,0);
            hout = HEADER_MOVE;
            buffer_write(&buffer_send,&hout,sizeof(char));
            char val = common->val;
            buffer_write(&buffer_send,&val,sizeof(char));
            res = send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
            
            pthread_mutex_unlock(&common->lock);
        }

        if((clock()-turntime)%CLOCKS_PER_SEC == 0){
            if(turn != common->turn){
                turn = common->turn;
                turntime = clock();
            }else if(clock()-turntime > 20*CLOCKS_PER_SEC){
                network_disconnect(common->sockets->data.socket);
            }
        }
    }
}

void* network_thread_listener(void* arg){
    threadcommon* common = (threadcommon*)arg;
    struct sockaddr_in client;
    int clientsock;
    while (common->all_terminate == 0) {
        int clientlen = sizeof(client);
        clientsock = accept(common->serversock, (struct sockaddr*)&client, &clientlen);
        if (clientsock < 0) {
            perror("Accept failed");
            continue;
        }
        //clientdata data;
        //data.socket = clientsock;
        clientdata* data = (clientdata*)malloc(sizeof(clientdata));
        data->socket = clientsock;

        //client.name = NULL;
        //common->sockets[common->socketcount++] = data;
        printf("tryna create a linked list over eere\n");
        common->sockets = client_insert(common->sockets,data);
        //common.sockets[common.socketcount++] = clientsock;

        printf("socket accept\n");

        // Enqueue the client socket
        enqueue(common->sockets,common);
    }
    close(common->serversock);
}

int main() {
    void* buffer_send[PACKET_MAX];
    pthread_t thread_pool[THREAD_POOL_SIZE];
    pthread_t actor, listener;

    setbuf(stdout, NULL);

    //struct sockaddr_in server, client;
    int res;
    int readSize;

    //int serversock, clientsock;

    threadcommon common;
    common.lock = PTHREAD_MUTEX_INITIALIZER;
    common.cond = PTHREAD_COND_INITIALIZER;
    common.task_count = 0;
    common.all_terminate = 0;
    common.socketcount = 0;
    common.turn = 0;
    common.val = 25;
    common.state = 0;
    common.sockets = NULL;
    
    // Create TCP socket
    common.serversock = socket(AF_INET, SOCK_STREAM, 0);
    if (common.serversock == -1) {
        printf("Creating socket failed\n");
        exit(1);
    }
    printf("Socket successfully created\n");
    
    common.server.sin_family = AF_INET;
    common.server.sin_addr.s_addr = INADDR_ANY;
    common.server.sin_port = htons(PORT);
    
    res = bind(common.serversock, (struct sockaddr *) &common.server, sizeof(common.server));
    if(res < 0){
        printf("Bind failed\n");
        exit(1);
    }
    printf("Bind was successfully completed\n");
    
    res = listen(common.serversock, BACKLOG);
    if(res != 0){
        printf("Listen failed\n");
        exit(1);
    }

    for (int i = 0; i < THREAD_POOL_SIZE; i++) pthread_create(&thread_pool[i], NULL, thread_worker, &common);
    pthread_create(&actor, NULL, network_thread_actor, &common);
    pthread_create(&actor, NULL, network_thread_listener, &common);
    //pthread_create(&listener, NULL, network_thread_listener, &common);
    printf("Server is listening on port %d\n", PORT);

    //int time = clock();

    // Accept connections
    pthread_join(actor,NULL);
    pthread_join(listener,NULL);
    //close(serversock);
    return 0;
}

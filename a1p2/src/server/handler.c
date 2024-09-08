#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include <math.h>
#include "../protocol.h"
#include "server.h"

void* handle_client(threadcommon* common, void* arg) {
    // get client information from thread void* arg
    cnode* clientptr = (cnode*)arg;
    clientdata* client = &clientptr->data;
    int socket = client->socket;

    // define i/o buffers
    char hin,hout;
    net_buffer buffer_send;
    net_buffer buffer_recv;
    char string_send[INPUT_MAX], string_recv[INPUT_MAX];
    int read_size, infractions = 0;

    printf("starting thread handler\n");

    // start recv loop
    while(common->all_terminate == 0 && client->terminate == 0){
        // reset and recv to i/o buffers, error check
        buffer_seek(&buffer_recv,0);
        buffer_seek(&buffer_send,0);
        memset(string_recv,'\0',sizeof(char)*INPUT_MAX);
        memset(string_send,'\0',sizeof(char)*INPUT_MAX);
        int size_in = recv(socket, buffer_recv.buffer, PACKET_MAX, 0);
        if(size_in < 0){
            printf("ERROR recv failed\n");
            break;
        }else if(size_in == 0){
            printf("lost connection to client\n");
            break;
        }
        // set end of buffer to \0 just in case, if i see 'Segmentation Fault' again i might cry
        buffer_recv.buffer[size_in] = '\0';

        // get packet header
        hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        // start packet stream because packets can be concantenated
        while(hin != '\0'){
            switch(hin){
                case HEADER_INFO:
                    // always when a client joins, receive their name and send join message out to all other clients
                    buffer_read_string(&buffer_recv,string_recv);
                    snprintf(client->name,NAME_MAX*sizeof(char),"%s",string_recv);
                    printf("%s connected\n",client->name);

                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s joined the game.",client->name);
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);
                break;

                case HEADER_TEXT:
                    // a chat message from a client
                    buffer_read_string(&buffer_recv,string_recv);

                    // send it back to all other players, include their name
                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s: %s",client->name,string_recv);

                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);
                break;

                case HEADER_MOVE:
                    // player moves, input is a string. game function pointers interpret input to say if its valid and update the game state
                    buffer_read_string(&buffer_recv,string_recv);
                    int move_err, move_post;
                    if(common->game.handle_move_check != NULL) move_err = common->game.handle_move_check(common,string_recv);
                    else{
                        move_err = 0;
                        //server isnt playing a game, just a text lobby
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        snprintf(string_send,INPUT_MAX*sizeof(char),"this server isnt hosting a game. use ':<message>' to send text, or 'quit' to leave");
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_TO);
                    }

                    move_post = 0;
                    if(clientptr != common->clients) move_err |= GAME_ERROR_SEQ; // turn sequence error, this doesnt depend on the game so it stays here

                    if(move_err == 0){
                        // rotate cnode* linked list to move this client to back of turn queue, then update game state
                        if(common->game.handle_move_update != NULL){
                            client_rotate(common);
                            move_post = common->game.handle_move_update(common,string_recv);
                        }else move_post = 0;
                    }else{
                        // disconnect client after 5 game infractions, even though they are technically impossible. these dont reset the turn timer so youll still get disconnected after 20 seconds anyway
                        if(infractions >= GAME_INFRACTION_LIMIT-1){
                            client->terminate = 1;
                        }
                        infractions++;

                        // concantenate and send error messages that apply
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        if((move_err&GAME_ERROR_OOB) == GAME_ERROR_OOB){
                            snprintf(string_send,INPUT_MAX*sizeof(char),"ERROR move '%s' out of bounds\n",string_recv);
                            buffer_write(&buffer_send,&hout,sizeof(char));
                            buffer_write_string(&buffer_send,string_send);
                        }
                        if((move_err&GAME_ERROR_SEQ) == GAME_ERROR_SEQ){
                            snprintf(string_send,INPUT_MAX*sizeof(char),"ERROR it's not your turn\n");
                            buffer_write(&buffer_send,&hout,sizeof(char));
                            buffer_write_string(&buffer_send,string_send);
                        }
                        if((move_err&GAME_ERROR_MAL) == GAME_ERROR_MAL){
                            snprintf(string_send,INPUT_MAX*sizeof(char),"ERROR malformed input data\n");
                            buffer_write(&buffer_send,&hout,sizeof(char));
                            buffer_write_string(&buffer_send,string_send);
                        }
                        snprintf(string_send,INPUT_MAX*sizeof(char),"game infraction %i/%i",infractions,GAME_INFRACTION_LIMIT);
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,string_send);
                        send(socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
                        break;
                    }

                    // handle next state, gameover state or next turn
                    if(move_post > 0){
                        // gameover state, val is less than 0. 
                        char* msg[] = {"you win :)","you lose :("};
                        // tell all clients WHO won the game
                        snprintf(string_send,INPUT_MAX*sizeof(char),"%s won the game.",client->name);

                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_ALL);

                        // tell the client that won, that they won
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[0]);
                        send(socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

                        // tell all the other clients that they lost
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[1]);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);

                        // kick everyone from the server
                        network_disconnect_all(common);

                        // reset game state
                        game_reset(common);
                    }else{
                        // send move packet to next client in queue, which is now the root node of common->clients linked list
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_MOVE;
                        //char vout = common->val;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        //buffer_write(&buffer_send,&vout,sizeof(char));
                        send(common->clients->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

                        // tell all other clients who is taking their turn
                        snprintf(string_send,INPUT_MAX*sizeof(char),"%s is taking their turn.",common->clients->data.name);
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),common->clients->data.socket,NETWORK_TARGET_EXCEPT);
                    }
                break;

                case HEADER_END:
                    // initiate clean shutdown of client thread, when it receives HEADER_END it means the user typed 'quit'
                    printf("%s disconnected\n",client->name);
                    client->terminate = 1;

                    // tell all other clients that this client has disconnected
                    buffer_seek(&buffer_send,0);
                    hout = HEADER_TEXT;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    snprintf(string_send,INPUT_MAX*sizeof(char),"%s left the game.",client->name);
                    buffer_write_string(&buffer_send,string_send);
                    send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),socket,NETWORK_TARGET_EXCEPT);
                break;

                default:
                    // always a malformed packet or out of bounds header or both, which means the client is faulty
                    printf("ERROR malformed packet or bad header from %s. sending them to the shadow realm\n",client->name);
                    network_disconnect(clientptr);
                    client->terminate = 1;
                break;
            }
            // get next header
            hin = *(char*)buffer_read(&buffer_recv,sizeof(char));
        }
    }

    // close socket and remove node from linked list
    close(socket);
    client_remove(common,clientptr);
    return NULL;
}

void* network_thread_actor(void* arg){
    // thread common data ptr, initialize static i/o buffers
    threadcommon* common = (threadcommon*)arg;
    net_buffer buffer_send;
    char hout;
    char* msg[] = {"you win, because everyone else is gone","timed out on your turn","timed out from no ping response"};
    char string_send[INPUT_MAX];

    // define timers
    int turntime = clock(), time = clock();
    cnode* turn = NULL; // turn client, to reset timer if it changes

    // start server control loop
    while(common->all_terminate == 0){
        // every 1 second, if the root node common->clients is not null. if its null, there are no players
        if(clock()-time > CLOCKS_PER_SEC && common->clients != NULL){
            // lock resources because this doesnt happen very often but its important
            pthread_mutex_lock(&common->lock);
            time = clock(); // reset clock

            // get client count in session
            int clientcount = client_count(common);
            if(common->state == GAME_STATE_GO){
                if(clientcount >= common->game.def[0]){
                    if(turn != common->clients){
                        // player has taken their turn or disconnected, so the turn timer is reset
                        turn = common->clients;
                        turntime = clock();
                    }else if(clock()-turntime > NETWORK_TIMEOUT_WAIT*CLOCKS_PER_SEC){
                        // send timeout message to client
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[1]);
                        send(common->clients->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

                        // tell all other clients that this client was timed out
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        snprintf(string_send,INPUT_MAX*sizeof(char),"%s was removed from the game.",common->clients->data.name);
                        buffer_write_string(&buffer_send,string_send);
                        send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),common->clients->data.socket,NETWORK_TARGET_EXCEPT);

                        // disconnect client
                        network_disconnect(common->clients);
                    }

                    // remind player that its their turn, in case client loses the packet even though its tcp and that wont happen.
                    buffer_seek(&buffer_send,0);
                    hout = HEADER_MOVE;
                    buffer_write(&buffer_send,&hout,sizeof(char));
                    send(common->clients->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
                }else{
                    if(clientcount == 1){
                        // there is only 1 player left in an active game state, so they win automatically but also get disconnected.
                        // send a win message
                        buffer_seek(&buffer_send,0);
                        hout = HEADER_TEXT;
                        buffer_write(&buffer_send,&hout,sizeof(char));
                        buffer_write_string(&buffer_send,msg[0]);
                        send(common->clients->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

                        network_disconnect(common->clients);
                    }
                    // reset game state
                    //common->val = common->valdef;
                    //common->state = GAME_STATE_WAIT;
                    game_reset(common);
                }
            }else if(common->state == GAME_STATE_WAIT && clientcount >= common->game.def[0]){
                // if there are enough players, set new game state and tell all clients the game is starting
                common->state = GAME_STATE_GO;
                printf("have enough players, starting!\n");

                buffer_seek(&buffer_send,0);
                hout = HEADER_TEXT;
                buffer_write(&buffer_send,&hout,sizeof(char));
                snprintf(string_send,INPUT_MAX*sizeof(char),"Game starting with %i players.",clientcount);
                buffer_write_string(&buffer_send,string_send);
                send_all(common,buffer_send.buffer,buffer_tell(&buffer_send),-1,NETWORK_TARGET_ALL);
            }else if(common->state == GAME_STATE_WAIT){
                // stop turn timer if game is in wait
                turn = NULL;
                turntime = clock();
            }
            pthread_mutex_unlock(&common->lock);
        }
    }
}

// listener thread handles incoming client connections
void* network_thread_listener(void* arg){
    // socket info from common data
    threadcommon* common = (threadcommon*)arg;
    struct sockaddr_in client;
    int clientsock;
    
    // start listener
    while (common->all_terminate == 0) {
        // blocking accept client (wait for client to connect)
        int clientlen = sizeof(client);
        clientsock = accept(common->serversock, (struct sockaddr*)&client, &clientlen);
        if (clientsock < 0) {
            perror("ERROR accept failed");
            continue;
        }

        int count = client_count(common);
        if(common->state == GAME_STATE_WAIT && count < THREAD_POOL_SIZE){
            // if game state is wait, add them to the cnode* linked list as the new root node
            char* user = "user";
            client_insert(common);
            strncpy(common->clients->data.name,user,sizeof(common->clients->data.name));
            common->clients->data.socket = clientsock;
            common->clients->data.terminate = 0;
            common->clients->data.ping = NETWORK_TIMEOUT_PING;

            net_buffer buffer_send;
            char hout, strout[INPUT_MAX];
            snprintf(strout,INPUT_MAX,"welcome to the game! currently playing %s",common->game.name);
            buffer_seek(&buffer_send,0);
            hout = HEADER_TEXT;
            buffer_write(&buffer_send,&hout,sizeof(char));
            buffer_write_string(&buffer_send,strout);
            send(clientsock,buffer_send.buffer,buffer_tell(&buffer_send),0);

            enqueue(common->clients,common);
        }else{
            // if not in wait, send a message that the session is in progress and then disconnect the player without starting their thread
            net_buffer buffer_send;
            char hout, *strout = "the server is not currently accepting connections";
            buffer_seek(&buffer_send,0);
            hout = HEADER_TEXT;
            buffer_write(&buffer_send,&hout,sizeof(char));
            buffer_write_string(&buffer_send,strout);
            hout = HEADER_END;
            buffer_write(&buffer_send,&hout,sizeof(char));
            send(clientsock,buffer_send.buffer,buffer_tell(&buffer_send),0);
            close(clientsock);
        }
    }
    // close server because network_thread_listener is the only thing that uses this socket, best its closed here
    close(common->serversock);
}
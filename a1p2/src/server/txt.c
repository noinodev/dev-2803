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
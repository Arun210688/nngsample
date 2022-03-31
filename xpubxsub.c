#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>

#define SERVER "server"
#define CLIENT "client"

#define CLIENTS 15
#define SERVERS 15

#define TOPICPREFIX "s"
#define TOPICLENGTH 3

static  long int publishData = 0;

typedef struct {
    //Or whatever information that you need
	 char *url;
     char *topic;
	 int topicLength;
} pubsub_struct;

void fatal(const char *func, int rv)
{
        fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
}

char* date(char *id)
{
		char *text =  malloc(11 * sizeof(char));;
		 
        //time_t now = time(&now);
       // struct tm *info = localtime(&now);
		publishData = publishData+1;
		//printf("GENERATED DATA %ld\n", publishData);
        //char *text = asctime(info);
		sprintf(text, "%s%ld",id, publishData);
		
        //text[strlen(text)-1] = '\0'; // remove '\n'
		//printf("GENERATED DATA string %s\n", text);
        return (text);
}

void *server(void *args)
{
		pubsub_struct *actual_args = args;
        nng_socket PubSock;
        int rvPub;

        if ((rvPub = nng_pub0_open(&PubSock)) != 0) {
                fatal("nng_pub0_open server", rvPub);
        }
		   
	
        if ((rvPub = nng_dial(PubSock, actual_args->url, NULL, 0)) < 0) {
                fatal("nng_listen server", rvPub);
        }
		
        for (;;) {
                char *d = date(actual_args->topic);
                printf("Server %s: PUBLISHING DATA %s \n",actual_args->topic, d);
                if ((rvPub = nng_send(PubSock, d, strlen(d) + 1, 0)) != 0) {
                        fatal("nng_send", rvPub);
				}
				free(d);
				
				usleep(500000);
				//free(d);
                }
                
                
        
		free(actual_args);
		pthread_exit(NULL);
}


void *client(void *args)
{
		pubsub_struct *actual_args = args;
        nng_socket SubSock;
        int rvSub;

        if ((rvSub = nng_sub0_open(&SubSock)) != 0) {
                fatal("nng_sub0_open client", rvSub);
        }

        
        if ((rvSub = nng_setopt(SubSock, NNG_OPT_SUB_SUBSCRIBE, actual_args->topic,actual_args->topicLength)) != 0) {
                fatal("nng_setopt client", rvSub);
        }
        if ((rvSub = nng_dial(SubSock, actual_args->url, NULL, 0)) != 0) {
                fatal("nng_dial client", rvSub);
        }
        for (;;) {
                char *buf = NULL;
                size_t sz;
                if ((rvSub = nng_recv(SubSock, &buf, &sz, NNG_FLAG_ALLOC)) != 0) {
                        fatal("nng_recv", rvSub);
                }
                printf("-------------------CLIENT %s : RECEIVED %s\n", actual_args->topic,buf); 
                nng_free(buf, sz);
				usleep(500000);
        }
		free(actual_args);
		pthread_exit(NULL);
}



void *device(void *args)
{
		pubsub_struct *actual_args = args;
        
		//subscriber
		nng_socket RawSubSock;
        int rvSubRaw;

        if ((rvSubRaw = nng_sub0_open_raw(&RawSubSock)) != 0) {
                fatal("nng_sub0_open_raw device", rvSubRaw);
        }


        if ((rvSubRaw = nng_listen(RawSubSock, "inproc://bar1", NULL, 0)) != 0) {
                fatal("device nng_listen", rvSubRaw);
        }
		
		
		//publisher
		nng_socket RawPubSock;
        int rvPubRaw;

        if ((rvPubRaw = nng_pub0_open_raw(&RawPubSock)) != 0) {
                fatal("nng_pub0_open_raw device", rvPubRaw);
        }
		   
		
        if ((rvPubRaw = nng_listen(RawPubSock, "inproc://bar2", NULL, 0)) < 0) {
                fatal("device nng_listen", rvPubRaw);
        }
	
	

	if (nng_device (RawPubSock,RawSubSock) != 0) {
		fatal("nng_device", rvPubRaw);
        
    }
		free(actual_args);
		pthread_exit(NULL);
}



int
main(const int argc, const char **argv)
{
	pthread_t device_id;
	pthread_t server_id[SERVERS];
	pthread_t client_id[CLIENTS];
	
	pubsub_struct *deviceargs = malloc(sizeof *deviceargs);
	pubsub_struct serverargs[SERVERS];
	pubsub_struct clientargs[CLIENTS];
	int i;
	
	char Serverurl[] = "inproc://bar1";
	char Clienturl[] = "inproc://bar2";
	char topic[] = TOPICPREFIX;
	
	
	pthread_create(&device_id, NULL, device, deviceargs);
	
	sleep(3);
	
	
	//list of Publishers
	
	for(i=0;i<SERVERS;i++)
	{
		
		serverargs[i].url = Serverurl;
		
		char *text =  malloc(3 * sizeof(char));
		sprintf(text, "%s%d%s",topic, i+1,",");
		serverargs[i].topic= text;
		
		serverargs[i].topicLength= TOPICLENGTH;
		
		pthread_create(&server_id[i], NULL, server, (void *)&serverargs[i]);
	}
	
	
	sleep(3);
	
	//list of subscribers	
	
	for(i=0;i<CLIENTS;i++)
	{
		
		clientargs[i].url = Clienturl;
		
		char *text =  malloc(3 * sizeof(char));
		sprintf(text, "%s%d%s",topic, i+1,",");
		clientargs[i].topic= text;
		clientargs[i].topicLength= TOPICLENGTH;
		
		pthread_create(&client_id[i], NULL, client, (void *)&clientargs[i]);
	}
	
	pthread_join(device_id,NULL);  
	
	for (i=0;i<SERVERS;i++)
	{
		pthread_join(server_id[i],NULL);  
	}
	
	
	for (i=0;i<CLIENTS;i++)
	{
		pthread_join(client_id[i],NULL);  
	}

    return 1;
}
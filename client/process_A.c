#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <netdb.h>

#define INPUT_FILE "demo_sharedmem_input.txt"
#define OUTPUT_FILE "demo_sharedmem_output.txt"
#define BUF_SIZE 1024
#define SHARED_KEY 9765
#define PORT 8087
#define SA struct sockaddr
#define SERVER_IP "192.168.160.115"

struct shared_seg{
    int process_a_status;
    int process_b_status;
    char buf[BUF_SIZE];
    char buf_b[BUF_SIZE];
};


void *handle_receive(void* arg)
{
    int connfd=*(int*)arg;

    int shm_id;
    struct shared_seg *receiver_pointer=(struct shared_seg*)malloc(sizeof(struct shared_seg));

	int flag = 1;

    FILE *output_logger = NULL;

    shm_id= shmget(SHARED_KEY, sizeof(char)*BUF_SIZE, 0644|IPC_CREAT);

    output_logger = fopen(OUTPUT_FILE, "a");

    receiver_pointer=shmat(shm_id, NULL, 0);

    if (output_logger != NULL)
    {
		while (flag )
		{
        	if(receiver_pointer->process_b_status == 0)
        	{
            	printf("PROCESS_A client Receiving message from process B Client\n %s\n", receiver_pointer->buf_b);
            	fprintf(output_logger,"%s\n", receiver_pointer->buf_b);
            	receiver_pointer->process_b_status=-1;
				flag = 0;
        	}
		}
    }

    printf("PROCESS_A client sending received message from process B client to the process A in server\n");
    write(connfd, receiver_pointer->buf_b, sizeof(receiver_pointer->buf_b));
    memset(receiver_pointer->buf_b, 0, sizeof(receiver_pointer->buf_b));
    shmdt(receiver_pointer);
    fclose(output_logger);
}

void *handle_sending(void *arg)
{
    int connfd = *(int*)arg;

    FILE *input_parser = NULL;
    char read_buffer[100];

    input_parser = fopen(INPUT_FILE, "r");
    printf("\nPROCESS_A client sending text  to PROCESS A server");
    while (fgets(read_buffer, 100, input_parser) != NULL)
    {
        write(connfd, read_buffer, sizeof(read_buffer));
        printf("PROCESS_A client . %s\n",read_buffer);
    }
    printf("\nPROCESS_A client message sent to PROCESS A server\n");
}

void *handle_receiving(void *arg)
{
    int connfd = *(int *)arg;
    char buffer[BUF_SIZE];
    char buff[100];

    int shm_id;
    struct shared_seg *sender_pointer=(struct shared_seg*)malloc(sizeof(struct shared_seg));

    printf("\nPROCESS_A client Receiving message from PROCESS A server.");
    do
    {
        memset(buff, 0, sizeof(buff));
        read(connfd, buff, sizeof(buff));
        strncat(buffer,buff, sizeof(char)*100);
        printf(".");
    }while(strncmp(buff,"exit",4) != 0);

    printf("\nPROCESS_A client Received from PROCESS A server\n");
    printf("\nPROCESS_A client Received from PROCESS A server is %s\n", buffer);

    printf("\nPROCESS_A client Preserving in shared_memory\n");
    shm_id= shmget(SHARED_KEY, sizeof(char)*BUF_SIZE, 0644|IPC_CREAT);

    sender_pointer=shmat(shm_id, NULL, 0);

    strncpy(sender_pointer->buf, buffer, sizeof(sender_pointer->buf));
    sender_pointer->process_a_status=0;
    shmdt(sender_pointer);
    printf("\nPROCESS_A client Preserved\n");
}

int main()
{
	int sockfd;
    struct sockaddr_in servaddr, cli;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("PROCESS_A client socket creation failed...\n");
        exit(0);
    }
    else
        printf("PROCESS_A client Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(PORT);
 
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("PROCESS_A client connection with the server failed...\n");
        exit(0);
    }
    else
	{
        printf("PROCESS_A client connected to the server..\n");
 	}
	
	pthread_t  shm_receiver_thread, socket_thread, socket_thread_2;

	pthread_create(&socket_thread, NULL, handle_sending, (void*)&sockfd);
    //pthread_create(&socket_thread_2, NULL, handle_receiving, (void*)&sockfd);
    //pthread_create(&shm_receiver_thread, NULL, handle_receive, (void*)&sockfd);

    pthread_join(socket_thread, NULL);
    //pthread_join(socket_thread_2, NULL);
    //pthread_join(shm_receiver_thread, NULL);

	close(sockfd);
	return 0;
}

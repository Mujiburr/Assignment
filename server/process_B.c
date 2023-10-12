#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define INPUT_FILE "demo_sharedmem_input_1.txt"
#define OUTPUT_FILE "demo_sharedmem_output_1.txt"
#define BUF_SIZE 1024
#define SHARED_KEY 8765
#define PORT 8088
#define SA struct sockaddr

struct shared_seg{
	int process_a_status;
	int process_b_status;
	char buf[BUF_SIZE];
	char buf_b[BUF_SIZE];
};


void *handle_receive(void* arg)
{
	int connfd=*(int *)arg;
	
	int shm_id;
    struct shared_seg *receiver_pointer=(struct shared_seg*)malloc(sizeof(struct shared_seg));

	int flag = 1;
    FILE *output_logger = NULL;

    shm_id= shmget(SHARED_KEY, sizeof(char)*BUF_SIZE, 0644|IPC_CREAT);

    output_logger = fopen(OUTPUT_FILE, "a");
    
	receiver_pointer=shmat(shm_id, NULL, 0);

    if (output_logger != NULL)
    {
		while (flag)
		{
			if(receiver_pointer->process_a_status == 0)
    		{
        		printf("PROCESS_B server Receiving message from process A Server\n %s\n", receiver_pointer->buf);
        		fprintf(output_logger,"%s\n", receiver_pointer->buf);
				receiver_pointer->process_a_status=-1;
				flag = 0;
    		}
		}
    }

	printf("PROCESS_B server sending received message from process B to the client in process B\n");	
	write(connfd, receiver_pointer->buf, sizeof(receiver_pointer->buf)); 
	memset(receiver_pointer->buf, 0, sizeof(receiver_pointer->buf));
    shmdt(receiver_pointer);
    fclose(output_logger);
}

void *handle_sending(void *arg)
{
	int connfd = *(int *)arg;

	FILE *input_parser = NULL;
    char read_buffer[100];

	input_parser = fopen(INPUT_FILE, "r");
	printf("\nPROCESS_B server sending text. to PROCESS B Client");
	while (fgets(read_buffer, 100, input_parser) != NULL)
	{
		write(connfd, read_buffer, sizeof(read_buffer)); 
		printf("\nPROCESS B Server. %s\n",read_buffer);
	}
	printf("\nPROCESS_B server message sent PROCESS B Client\n");
}

void *handle_receiving(void *arg)
{
	int connfd = *(int *)arg;
	char buffer[BUF_SIZE];
	char buff[100];

	int shm_id; 
	struct shared_seg *sender_pointer=(struct shared_seg*)malloc(sizeof(struct shared_seg));

	printf("\nPROCESS_B server Receiving message from PROCESS B client.");
	do
	{	
		memset(buff, 0, sizeof(buff));
		read(connfd, buff, sizeof(buff));
		strncat(buffer,buff, sizeof(char)*100);
		printf(".");
	}while(strncmp(buff,"exit",4) != 0);

	printf("\nPROCESS_B server Received from PROCESS B client\n");
	printf("\nPROCESS_B server Message Received from PROCESS B client is %s\n", buffer);
	printf("\nPROCESS_B server Preserving in shared_memory\n");
	shm_id= shmget(SHARED_KEY, sizeof(char)*BUF_SIZE, 0644|IPC_CREAT);
	
	sender_pointer=shmat(shm_id, NULL, 0);

	strncpy(sender_pointer->buf_b, buffer, sizeof(sender_pointer->buf_b));
	sender_pointer->process_b_status=0;
    shmdt(sender_pointer);
	printf("\nPROCESS_B server Preserved\n");
}

int main(int argc, char*argv[])
{
	int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
	const int enable = 1;
	// Change the server IP
	const char *server_ip = argv[1];

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
    	printf("PROCESS_B server socket creation failed...\n"); 
    	exit(0); 
    } 
    else
    	printf("PROCESS_B server Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    	printf("PROCESS_B server setsockopt(SO_REUSEADDR) failed");
    
	// assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(server_ip); 
    servaddr.sin_port = htons(PORT); 
   
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
    	printf("PROCESS_B server socket bind failed...\n"); 
    	exit(0); 
    } 
    else
    	printf("PROCESS_B server Socket successfully binded..\n"); 
   
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
    	printf("PROCESS_B server Listen failed...\n"); 
    	exit(0); 
    } 
    else
    	printf("PROCESS_B Server listening..\n"); 
    len = sizeof(cli); 
   
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
    	printf("PROCESS_B server accept failed...\n"); 
    	exit(0); 
    } 
    else
       	printf("PROCESS_B server accepted the client...\n"); 
	
	pthread_t shm_receiver_thread, socket_thread, socket_thread_2;
	
	pthread_create(&shm_receiver_thread, NULL, handle_receive, (void*)&connfd);
	//pthread_create(&socket_thread, NULL, handle_sending, (void*)&connfd);
	//pthread_create(&socket_thread_2, NULL, handle_receiving, (void*)&connfd);
	
	pthread_join(shm_receiver_thread, NULL);
	//pthread_join(socket_thread, NULL);
	//pthread_join(socket_thread_2, NULL);
   
    close(sockfd);
	return 0;
}

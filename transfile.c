#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#define INFO "settings.data"

struct settings {
	int  port;
	char host[10];
};

void settings();
void save_data();
void load_data();
int  start_client();
int  start_server();
int  send_file(int socket, char *fname);
int  receive_file(int socket);

struct settings info;

int main() {
	int choice;

	while (1) {
		printf("\n\t\t------========[ Menu ]========------\n");
	  printf("1 - Send file\n");
	  printf("2 - Receive file\n");
	  printf("3 - Settings\n");
	  printf("0 - Exit\n");
	  printf("-> ");
	  scanf("%d", &choice);

	  if ((choice < 0) || (choice > 3))
	    printf("\n[!!!] Error\n\n");
	 	if (choice == 0)
			exit(0);
	  else if (choice == 1)
	    start_client();
		else if (choice == 2)
		  start_server();
		else if (choice == 3)
		  settings();
	}
}

int start_client() {
		int  n = 0;
		int  socket_desc;
		char fname[100];
		struct sockaddr_in server;

		printf("Enter filename to send: ");
		scanf("%s", fname);

    load_data();

  	FILE *data;
  	data = fopen(fname, "r");
  	if (data == NULL) {
			printf("Error: There is no such file - %s.\n", fname);
			exit(0);
  	} else fclose(data);

  	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  	if (socket_desc == -1) {
  		printf("Failed to create socket.");
  		return 1;
  	}

  	memset(&server, 0, sizeof(server));
  	server.sin_addr.s_addr = inet_addr(info.host);
  	server.sin_family = AF_INET;
  	server.sin_port = htons(info.port);

  	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		puts("Failed to connect. Check the specified port.");
  		close(socket_desc);
  		return 1;
  	}

  	char start[1024] = "start";
  	n = write(socket_desc, start, strlen(start));
  	if (n > 0)
  		send_file(socket_desc, fname);

  	close(socket_desc);
  	fflush(stdout);
}

int start_server() {
  int  c;
  int  n = 0;
  int  socket_desc;
  int  new_socket;
  int  read_size;
  char start[1024] = "start";
  char buf[1024];
	struct sockaddr_in server, client;

  load_data();

  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1)
  	return 0;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(info.port);

  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    return 1;
  else
  	printf("Server is run... \n");

  while (1) {
    listen(socket_desc, 5);
    c = sizeof(struct sockaddr_in);

    new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    fflush(stdout);
    if (new_socket < 0)
      return 1;

		bzero(buf, 1024);
    n = recv(new_socket, buf, 1024, 0);
		if (strncmp(buf, start, strlen(start)) == 0) {
			receive_file(new_socket);
			close(new_socket);
    } else return -1;
  }
}

void settings() {
	int  fd;
	int  port;
	int  choice;
	char host[10];
	struct settings input;

	while (choice != 3) {
		printf("\n\t\t------========[ Menu Settings ]========------\n");
	  printf("1 - Show config\n");
	  printf("2 - Edit config\n");
	  printf("3 - Back\n");
	  printf("-> ");
		scanf("%d", &choice);

		if ((choice < 1) || (choice > 3))
	    printf("\n[!!!] Error\n\n");

	  if (choice == 1) {					/* choice = 1 Show data */
			struct settings output;

			fd = open(INFO, O_RDWR);
		  if (fd == -1)	{
				printf("\nSettings file does not exist.\n");
				save_data();
			} else {
		  	read(fd, &(output), sizeof(struct settings));
		  	printf("\nPort = %d\nHost = %s\n", output.port, output.host);
			}
		  close(fd);
		} else if (choice == 2) {				/* choice = 2  Edit\Save data */
	    save_data();
		}
	}
}

void save_data() {
	int  fd;
	int  port;
	int  choice;
	char host[10];
	struct settings input;

	printf("\nEnter Port and Host\n");
	printf("This data is saved a file 'settings.data'\n");

	printf("\nEnter port: ");
	scanf("%d", &port);

	printf("Enter host: ");
	scanf("%s", host);

	fd = open(INFO, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);

	input.port = port;
	strcpy(input.host, host);

	printf("\nPort = %d, Host = %s\n", input.port, input.host);
	write(fd, &input, sizeof(struct settings));

	close(fd);

	printf("\nData saved succesfully!\n");
}

void load_data() {
  int fd = open(INFO, O_RDWR);
  if (fd == -1)
    exit(0);

  read(fd, &(info), sizeof(struct settings));
  close(fd);
}

int send_file(int socket, char *fname) {
  FILE  *file;
	off_t size;
	int   stata;
  int   read_size;
  char  send_buffer[10240];
  struct stat st;

  file = fopen(fname, "r");
  if (file == NULL)
    printf("\nError opening file. Perhaps it doesn't exit.");

  if (stat(fname, &st) == -1)
    return st.st_size;
  size = st.st_size;

  printf("\nSending size and file name..\n");
  write(socket, (void *)&size, sizeof(int));
  write(socket, fname, sizeof(char *));

  printf("Sending file...\n");
  while(!feof(file)) {
    read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, file);
    do {
      stata = write(socket, send_buffer, read_size);
    } while (stata < 0);
    bzero(send_buffer, sizeof(send_buffer));
  }
  fclose(file);
  printf("\nFile sent successfully.\n");

  return 0;
}

int receive_file(int socket) {
	FILE *file;
	int  stat;
	int  size = 0;
	int  read_size;
	int  write_size;
	int  recv_size = 0;
	char filearray[10241];
	char fname[1024];

	stat = read(socket, &size, sizeof(int));
	read(socket, &fname, sizeof(char *));

	file = fopen(fname, "w");
	if (file == NULL)
	  return -1;

	struct timeval timeout = {10,0};
	fd_set fds;
	int buffer_fd;

	while(recv_size < size) {
	  FD_ZERO(&fds);
	  FD_SET(socket, &fds);

	  buffer_fd = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
	  if (buffer_fd < 0) return 1;
	  if (buffer_fd == 0) return 1;
	  if (buffer_fd > 0) {
	    do {
	      read_size = read(socket, filearray, 10241);
	    } while(read_size < 0);

	    write_size = fwrite(filearray, 1, read_size, file);

	    if (read_size != write_size) { return 1; }
	    recv_size += read_size;
	   }
	 }
	fclose(file);
	printf("FIle '%s' successfully received. (%d bytes)\n", fname, size);
	bzero(fname, sizeof(fname));

	return 0;
}

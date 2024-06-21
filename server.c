#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 1234

void
worker_process(int client_socket, struct sockaddr_in *client_addr_p)
{
	union {
		uint32_t hl;
		uint8_t byte[4];
	} client_addr_s;
	char header[BUFSIZ], buf[BUFSIZ];
	ssize_t n_read, n_write;

	client_addr_s.hl = ntohl(client_addr_p->sin_addr.s_addr);
	snprintf(header, sizeof(header), "%u.%u.%u.%u (%u)", client_addr_s.byte[3], client_addr_s.byte[2], client_addr_s.byte[1], client_addr_s.byte[0], ntohs(client_addr_p->sin_port));
	printf("%s: accepted connection\n", header);

	while (1) {
		n_read = read(client_socket, buf, sizeof(buf));
		printf("%s: ", header);
		if (n_read == 0) {
			puts("disconnected");
			break;
		} else if (n_read == -1) {
			perror("read()");
			break;
		}
		fwrite(buf, n_read, 1, stdout);
		snprintf(buf, sizeof(buf), "Received %ld bytes\n", n_read);
		n_write = write(client_socket, buf, strlen(buf));
		if (n_write == -1) {
			perror("write()");
			break;
		}
	}
	exit(0);
}

int
main(void)
{
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		perror("socket()");
		return 1;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("bind()");
		return 1;
	}

	if (listen(server_socket, 1) < 0) {
		perror("listen()");
		return 1;
	}
	printf("Server is listening at port %d. Ctrl-C to terminate.\n", PORT);

	int client_socket;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = (socklen_t) sizeof(client_addr);
	while (1) {
		client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len);
		if (client_socket == -1) {
			perror("accept()");
			continue;
		}

		if (fork() == 0) {
			worker_process(client_socket, &client_addr);
		}
	}

	return 0;
}

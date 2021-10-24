#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "./types.h"

const char HTTP_RESPONSE_HEADERS[] = "\
HTTP/1.1 200 OK\n\
Connection: close\n\
Content-Type: text/plain\n\
";

const char PROM_FORMAT[] = "\
# HELP sensor_co2_ppm Shows how many parts per million of the ambient atmosphere are CO2\n\
# TYPE sensor_co2_ppm gauge\n\
sensor_co2_ppm %d\n\
\n\
# HELP sensor_voc_ppm Shows how many parts per million of the ambient atmosphere are VOC\n\
# TYPE sensor_voc_ppm gauge\n\
sensor_voc_ppm %d\n";

const char ALLOWED_ENDPOINT[] = "GET /metrics HTTP/1.1";

void *listen_and_serve(void *args) {
	// running always needs to be dereferenced so it knows that
	// the process has received a SIGINT.
    struct http_thread_args* harg = ((http_thread_args *)args);
	int *running = harg->running;

    char address[17] = "0.0.0.0";
    strncpy(address, harg->address, 16);
    uint16_t port = harg->port;

	struct sockaddr_in host_addr;
	struct sockaddr_in remote_addr;

	*(harg->sock_fd) = socket(AF_INET, SOCK_STREAM, 0);
	// The function only needs its value here-on-out - no need to dereference
	// on each call.
	int sockfd = *(harg->sock_fd);

	if (sockfd < 0) {
		perror("cannot acquire socket");
		exit(1);
	}
	int yes = 1;
	// lose the pesky "Address already in use" error message
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	host_addr.sin_family = AF_INET;	  // host byte order
	host_addr.sin_addr.s_addr = inet_addr(address);
	host_addr.sin_port = htons(port); // short, network byte order
	memset(&(host_addr.sin_zero), '\0', 8); // zero the rest of the struct

	if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) != 0) {
		perror("cannot bind to address");
		exit(1);
	}

	if (listen(sockfd, harg->backlog) != 0) {
		perror("listen");
		exit(1);
	}

	socklen_t sin_size;
	int		  conn_fd;
	sin_size = sizeof(struct sockaddr_in);
	while (*running) {
		conn_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &sin_size);

		char remote_body[1024] = {0};
		if (recv(conn_fd, remote_body, 1023, 0) == -1) {
			perror("request read");
			exit(1);
		}
		// Not receiving 'GET /metrics' -> 404
		if (strncmp(remote_body, ALLOWED_ENDPOINT, strlen(ALLOWED_ENDPOINT)) != 0) {
			printf("Received:\n%s\n", remote_body);
			send(conn_fd, "HTTP/1.1 404 Not Found\nContent-Length: 0\n", 42, 0);
			close(conn_fd);
			continue;
		}

		// Print out data
		char length_header[100] = {0};
		char body[512] = {0};

		const ccs811_measurement_t *data = ((http_thread_args *)args)->sensor_data;
		snprintf(body, 511, PROM_FORMAT, data->eco2, data->tvoc);
		snprintf(length_header, 99, "Content-Length: %lu\n\r\n", strlen(body));

		send(conn_fd, HTTP_RESPONSE_HEADERS, strlen(HTTP_RESPONSE_HEADERS), 0);
		send(conn_fd, length_header, strlen(length_header), 0);
		send(conn_fd, body, strlen(body), 0);

		// Kill the connection
		close(conn_fd);
	}

	return NULL;
}
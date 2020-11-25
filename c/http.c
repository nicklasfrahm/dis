#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main(void) {
  // Get the port number.
  const char *port_str = getenv("PORT");
  if (port_str == NULL) {
    port_str = "5555";
  }
  int port = atoi(port_str);

  // Creating socket file descriptor.
  int server_fd;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("creating socket failed");
    exit(EXIT_FAILURE);
  }

  // Configuring socket options.
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setting socket options failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port.
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // Listen to incoming connections on server socket.
  if (listen(server_fd, 100) < 0) {
    perror("listening for connections failed");
    exit(EXIT_FAILURE);
  }

  int addr_len = sizeof(address);
  printf("Service online: http://nfrah16.dedyn.io:%d\n", port);

  while (1) {
    char buffer[1024] = {0};
    int new_socket;
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addr_len)) < 0) {
      perror("accepting connection failed");
      exit(EXIT_FAILURE);
    }
    int valread = recv(new_socket, buffer, 1024, 0);

    printf("%s\n", buffer);

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"data\":\"Hello HTTP!\"}\r\n";
    send(new_socket, response, strlen(response), 0);
    if (shutdown(new_socket, SHUT_RDWR)) {
      perror("error shutting down socket");
    }
  }

  return 0;
}
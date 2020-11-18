#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
  // Get the port number.
  const char* port = getenv("PORT");
  if (port == NULL) {
    port = "8080";
  }

  printf("Service online: tcp://localhost:%s\n", port);

  // Create a new socket.
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
}
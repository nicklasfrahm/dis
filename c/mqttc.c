#define _GNU_SOURCE

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define FLAG_CLEAN_SESSION (1 << 1)
#define MQTT311 4
#define CLIENT_ID "KASPER_NICKLAS"

typedef enum MESSAGE_TYPE { CONNECT = 1, CONNACK, PUBLISH } message_type_t;
typedef enum ERROR_TYPE {
  SUCCESS,
  INVALID_ARGUMENTS,
  SOCKET_CREATE,
  CONNECTION_FAILED
} error_type_t;

int mqtt_create(char** message_buffer, message_type_t message_type, char* topic,
                char* message) {
  char control_header = (message_type << 4);
  // Length of the variable header in bytes.

  char header_length = 0;

  switch (message_type) {
    case CONNECT: {
      char* protocol_name = "MQTT";
      uint16_t protocol_name_length = strlen(protocol_name);
      char protocol_version = MQTT311;
      char connect_flags = FLAG_CLEAN_SESSION;
      uint16_t keep_alive = 30;
      const char* client_id = CLIENT_ID;
      uint16_t client_id_length = strlen(client_id);

      header_length += protocol_name_length + sizeof(protocol_name_length);
      header_length += sizeof(protocol_version);
      header_length += sizeof(connect_flags);
      header_length += sizeof(keep_alive);
      header_length += client_id_length + sizeof(client_id_length);

      char message_length =
          sizeof(control_header) + sizeof(header_length) + header_length;
      *message_buffer = (char*)malloc(message_length);

      char* pointer = *message_buffer;
      int length = 0;
      char temp[2] = {0};

      // Add control header.
      length = sizeof(control_header);
      memcpy(pointer, &control_header, length);
      pointer += length;

      // Add message length.
      length = sizeof(header_length);
      memcpy(pointer, &header_length, length);
      pointer += length;

      // Add protocol name length.
      length = sizeof(protocol_name_length);
      temp[0] = protocol_name_length >> 8;
      temp[1] = protocol_name_length;
      memcpy(pointer, temp, length);
      pointer += length;

      // Add protocol name.
      length = protocol_name_length;
      memcpy(pointer, protocol_name, length);
      pointer += length;

      // Add protocol version.
      length = sizeof(protocol_version);
      memcpy(pointer, &protocol_version, length);
      pointer += length;

      // Add connect flags.
      length = sizeof(connect_flags);
      memcpy(pointer, &connect_flags, length);
      pointer += length;

      // Add keep alive.
      length = sizeof(keep_alive);
      temp[0] = keep_alive >> 8;
      temp[1] = keep_alive;
      memcpy(pointer, temp, length);
      pointer += length;

      // Add client ID length.
      length = sizeof(client_id_length);
      temp[0] = client_id_length >> 8;
      temp[1] = client_id_length;
      memcpy(pointer, temp, length);
      pointer += length;

      // Add client ID length.
      length = client_id_length;
      memcpy(pointer, client_id, length);
      pointer += length;
      break;
    }
    case PUBLISH: {
      asprintf(message_buffer, "%c", control_header);
      break;
    }
    default: {
      printf("error: something broke");
      exit(1);
    }
  }
}

int main(int argc, char const* argv[]) {
  const char* binary = argv[0];
  if (argc != 5) {
    printf("Usage: %s <host> <port> <topic> <message>\n", binary);
    return INVALID_ARGUMENTS;
  }

  // Get command line arguments.
  const char* host = argv[1];
  const uint8_t port = atoi(argv[2]);
  const char* topic = argv[3];
  const char* message = argv[4];
  printf(
      "====================\n"
      "Host: %s\n"
      "Port: %d\n"
      "Topic: %s\n"
      "Message: %s\n"
      "====================\n",
      host, port, topic, message);

  char* send_buffer;
  mqtt_create(&send_buffer, CONNECT, NULL, NULL);

  const int fixed_header_length = 2;
  for (int i = 0; i < send_buffer[1] + fixed_header_length; ++i) {
    printf("%02X ", send_buffer[i]);
  }
  printf("\n");
  // fflush(stdout);

  // Create TCP client socket.
  printf("Creating socket ...\n");
  int clt = 0;
  if ((clt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("error: failed to create socket\n");
    return SOCKET_CREATE;
  }

  // Configure the server address and convert IPv4 from text to binary form.
  printf("Configuring server address ...\n");
  struct sockaddr_in srv;
  srv.sin_family = AF_INET;
  srv.sin_port = htons(port);
  srv.sin_addr.s_addr = inet_addr(host);

  // Connect to the server via TCP.
  printf("Connecting to server ...\n");
  if (connect(clt, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
    printf("error: failed to connect to server\n");
    return CONNECTION_FAILED;
  }

  printf("Connected\n");

  // TODO:

  // char* = "Hello from client";

  // char buffer[1024] = {0};

  // send(client_socket, hello, strlen(hello), 0);
  // printf("Hello message sent\n");
  // ssize_t valread = read(client_socket, buffer, 1024);
  // printf("%s\n", buffer);

  return SUCCESS;
}

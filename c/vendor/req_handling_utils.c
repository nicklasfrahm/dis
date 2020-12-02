#include <errno.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

typedef struct {
  char *name, *value;
} header_t;

static header_t reqhdr[17] = {{(char *)"\0"}, {(char *)"\0"}};
static char *buf;
char *base64_encode(const void *buf, size_t size);
char *method,  // "GET" or "POST"
    *uri,      // "/index.html" things before '?'
    *qs,       // "a=1&b=2"     things after  '?'
    *prot;     // "HTTP/1.1"

char *payload;  // for POST
int payload_size;

char *request_header(const char *name);

/**
 * Extracts the value from a HTTP header field
 * @param name The name string for the header field.
 * @returns the a pointer to the extracted value.
 */
char *request_header(const char *name) {
  header_t *h = reqhdr;
  while (h->name) {
    if (strcmp(h->name, name) == 0) return h->value;
    h++;
  }
  return NULL;
}

/**
 * Calculate the sha1 of the key concatenated with the special websocket
 * protocol string.
 * @param key The key string received from the client
 * @param len The length of the key in bytes
 * @param dest a pointer to a 20 byte array where the resulting message digest
 * will be written.
 * @returns 0 on success, an error code otherwise.
 */
int websocket_sha(const char *key, unsigned int key_len, unsigned char *dest) {
  const static char WS_SHA_STRING[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA_CTX context;
  if (SHA1_Init(&context) && SHA1_Update(&context, key, key_len) &&
      SHA1_Update(&context, WS_SHA_STRING, strlen(WS_SHA_STRING)) &&
      SHA1_Final(dest, &context)) {
    return 0;
  } else {
    return EINVAL;
  }
}

/**
 * Encodes the given data with base64.
 * @param buf The binary input data
 * @param size The size of input data in bytes
 * @returns the encoded nul-terminated data, as a string.
 */
char *base64_encode(const void *buf, size_t size) {
  static const char base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  char *str = (char *)malloc((size + 3) * 4 / 3 + 1);

  char *p = str;
  const unsigned char *q = (const unsigned char *)buf;
  size_t i = 0;

  while (i < size) {
    int c = q[i++];
    c *= 256;
    if (i < size) c += q[i];
    i++;

    c *= 256;
    if (i < size) c += q[i];
    i++;

    *p++ = base64[(c & 0x00fc0000) >> 18];
    *p++ = base64[(c & 0x0003f000) >> 12];

    if (i > size + 1)
      *p++ = '=';
    else
      *p++ = base64[(c & 0x00000fc0) >> 6];

    if (i > size)
      *p++ = '=';
    else
      *p++ = base64[c & 0x0000003f];
  }

  *p = 0;

  return str;
}

/**
 * Receive data at a socket and parse the HTTP request with respect to
 * HTPP method, URI and Header field.
 * @param socket_h The socket to be received on
 * @param size The size of input data in bytes
 * @returns the encoded nul-terminated data, as a string.
 */
void respond(int socket_h) {
  int rcvd;

  buf = (char *)malloc(65535);
  rcvd = recv(socket_h, buf, 65535, 0);

  if (rcvd < 0)  // receive error
    fprintf(stderr, ("recv() error\n"));
  else if (rcvd == 0)  // receive socket closed
    fprintf(stderr, "Client disconnected upexpectedly.\n");
  else  // message received
  {
    buf[rcvd] = '\0';

    method = strtok(buf, " \t\r\n");
    uri = strtok(NULL, " \t");
    prot = strtok(NULL, " \t\r\n");

    fprintf(stderr, "\x1b[32m+ [%s] %s\x1b[0m\n", method, uri);

    if (qs = strchr(uri, '?')) {
      *qs++ = '\0';  // split URI
    } else {
      qs = uri - 1;  // use an empty string
    }

    header_t *h = reqhdr;
    char *t, *t2;

    while (h < reqhdr + 16) {
      char *k, *v, *t;
      k = strtok(NULL, "\r\n: \t");
      if (!k) break;
      v = strtok(NULL, "\r\n");
      while (*v && *v == ' ') v++;
      h->name = k;
      h->value = v;
      h++;
      fprintf(stderr, "  [HDR] %s: %s\n", k, v);
      t = v + 1 + strlen(v);
      if (t[1] == '\r' && t[2] == '\n') break;
    }
    t++;  // now the *t shall be the beginning of user payload
    t2 =
        request_header("Content-Length");  // and the related header if there is
    payload = t;
    payload_size = t2 ? atol(t2) : (rcvd - (t - buf));

    const char *wsUri = "/websocket";
    if (strlen(uri) == strlen(wsUri) && strcmp(uri, wsUri)) {
      // Extract Sec-WebSocket-Key value from header
      char *websock_key = (char *)malloc(sizeof(char) * 128);
      websock_key = request_header("Sec-WebSocket-Key");

      // Calculate the sha1 hash of the key
      unsigned char hash[SHA_DIGEST_LENGTH];
      websocket_sha(websock_key, strlen(websock_key), hash);

      // Convert to Base 64 encoding
      char *base64;
      base64 = base64_encode(hash, sizeof(hash));

      // ADD YOUR CODE TO ESTABLISH THE WEBSOCKET CONNECTION HERE !!!
    } else {
      FILE *fp = (FILE *)malloc(sizeof(FILE));
      if (strstr(uri, "./") != NULL) {
        const char *response_header =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Bad Request";
        fprintf(stderr, "\x1b[31m- [%d] %s\x1b[0m\n", 400, uri);
      } else if (strcmp(uri, "/") == 0) {
        fp = fopen("c/public/index.html", "r");
      } else {
        char *filename = (char *)malloc(sizeof(char) * 1024);
        snprintf(filename, sizeof(char) * 1024, "c/public%s", uri);
        fp = fopen(filename, "r");
        free(filename);
      }

      if (fp != NULL) {
        // Send response header.
        char *res_header = (char *)malloc(sizeof(char) * 50);
        const char *res_header_tpl =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n\r\n";
        const char *favicon = "/favicon.ico";
        if (strcmp(uri, favicon) == 0) {
          snprintf(res_header, sizeof(char) * 50, res_header_tpl,
                   "image/x-icon");
        } else {
          snprintf(res_header, sizeof(char) * 50, res_header_tpl, "text/html");
        }
        printf(res_header);
        send(socket_h, res_header, strlen(res_header), 0);

        // Stream file.
        char c = getc(fp);
        while (c != EOF && c != '\0') {
          send(socket_h, &c, 1, 0);
          c = getc(fp);
        }

        // Close file.
        fclose(fp);
        fprintf(stderr, "\x1b[32m- [%d] %s\x1b[0m\n", 200, uri);
      } else {
        const char *response_header =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Not Found";
        send(socket_h, response_header, strlen(response_header), 0);
        fprintf(stderr, "\x1b[31m- [%d] %s\x1b[0m\n", 404, uri);
      }
      if (shutdown(socket_h, SHUT_RDWR)) {
        perror("error shutting down socket");
      }
    }
  }
}
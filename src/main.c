#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 6379
#define READ_BUF_SIZE 4096

static int parse_port(int argc, char **argv) {
  int port = DEFAULT_PORT;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--port") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "error: --port requires a value\n");
        exit(1);
      }
      char *end = NULL;
      long value = strtol(argv[i + 1], &end, 10);
      if (end == argv[i + 1] || *end != '\0' || value <= 0 || value > 65535) {
        fprintf(stderr, "error: invalid port '%s'\n", argv[i + 1]);
        exit(1);
      }
      port = (int)value;
      i++;
    } else {
      fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
      exit(1);
    }
  }

  return port;
}

static void trim_crlf(char *line) {
  size_t len = strlen(line);
  while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
    line[len - 1] = '\0';
    len--;
  }
}

static bool is_ping(const char *line) {
  return strcmp(line, "PING") == 0;
}

static int create_listen_socket(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    close(fd);
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((uint16_t)port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(fd);
    return -1;
  }

  if (listen(fd, SOMAXCONN) < 0) {
    perror("listen");
    close(fd);
    return -1;
  }

  return fd;
}

static ssize_t read_line(int fd, char *buf, size_t cap) {
  size_t total = 0;

  while (total + 1 < cap) {
    ssize_t n = recv(fd, buf + total, 1, 0);
    if (n == 0) {
      return total == 0 ? 0 : (ssize_t)total;
    }
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (buf[total] == '\n') {
      buf[total + 1] = '\0';
      return (ssize_t)(total + 1);
    }
    total += (size_t)n;
  }

  errno = EMSGSIZE;
  return -1;
}

static int handle_client(int client_fd) {
  char buf[READ_BUF_SIZE];
  ssize_t n = read_line(client_fd, buf, sizeof(buf));
  if (n < 0) {
    perror("read");
    return -1;
  }
  if (n == 0) {
    return 0;
  }

  trim_crlf(buf);

  const char *response;
  if (is_ping(buf)) {
    response = "PONG\n";
  } else {
    response = "ERR unknown command\n";
  }

  size_t len = strlen(response);
  size_t sent = 0;
  while (sent < len) {
    ssize_t w = send(client_fd, response + sent, len - sent, 0);
    if (w < 0) {
      perror("send");
      return -1;
    }
    sent += (size_t)w;
  }

  return 0;
}

int main(int argc, char **argv) {
  int port = parse_port(argc, argv);
  int listen_fd = create_listen_socket(port);
  if (listen_fd < 0) {
    return 1;
  }

  printf("Listening on port %d\n", port);

  for (;;) {
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    if (handle_client(client_fd) < 0) {
      fprintf(stderr, "client error\n");
    }

    close(client_fd);
  }

  close(listen_fd);
  return 0;
}

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "unknown"
#endif

#define OUTPUT_LIMIT (4u * 1024u * 1024u)

typedef struct {
  char *data;
  size_t length;
  size_t capacity;
} Buffer;

static void
usage(FILE *out)
{
  fputs("usage: rawwin [-v]\n", out);
}

static int
buffer_append(Buffer *buffer, const void *data, size_t length)
{
  size_t needed, capacity;
  char *next;

  if (!length)
    return 0;
  if (buffer->length > OUTPUT_LIMIT - length)
    return -1;
  needed = buffer->length + length + 1u;
  if (needed > buffer->capacity)
  {
    capacity = buffer->capacity ? buffer->capacity : 4096u;
    while (capacity < needed)
    {
      if (capacity > OUTPUT_LIMIT / 2u)
      {
        capacity = OUTPUT_LIMIT + 1u;
        break;
      }
      capacity *= 2u;
    }
    if (capacity > OUTPUT_LIMIT + 1u)
      return -1;
    next = realloc(buffer->data, capacity);
    if (!next)
      return -1;
    buffer->data = next;
    buffer->capacity = capacity;
  }
  memcpy(buffer->data + buffer->length, data, length);
  buffer->length += length;
  buffer->data[buffer->length] = '\0';
  return 0;
}

static int
wait_child(pid_t pid)
{
  int status;

  while (waitpid(pid, &status, 0) < 0)
  {
    if (errno == EINTR)
      continue;
    return -1;
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static int
capture_command(char *const argv[], FILE *input, Buffer *output)
{
  int pipefd[2];
  pid_t pid;
  char chunk[4096];
  ssize_t count;
  int status;

  if (pipe(pipefd) < 0)
  {
    fprintf(stderr, "rawwin: pipe: %s\n", strerror(errno));
    return -1;
  }
  pid = fork();
  if (pid < 0)
  {
    fprintf(stderr, "rawwin: fork: %s\n", strerror(errno));
    close(pipefd[0]);
    close(pipefd[1]);
    return -1;
  }
  if (pid == 0)
  {
    if (input && dup2(fileno(input), STDIN_FILENO) < 0)
      _exit(126);
    if (dup2(pipefd[1], STDOUT_FILENO) < 0)
      _exit(126);
    close(pipefd[0]);
    close(pipefd[1]);
    execvp(argv[0], argv);
    fprintf(stderr, "rawwin: cannot execute %s: %s\n", argv[0], strerror(errno));
    _exit(errno == ENOENT ? 127 : 126);
  }

  close(pipefd[1]);
  memset(output, 0, sizeof(*output));
  while ((count = read(pipefd[0], chunk, sizeof(chunk))) != 0)
  {
    if (count < 0)
    {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "rawwin: read: %s\n", strerror(errno));
      close(pipefd[0]);
      (void)wait_child(pid);
      free(output->data);
      memset(output, 0, sizeof(*output));
      return -1;
    }
    if (buffer_append(output, chunk, (size_t)count) < 0)
    {
      fputs("rawwin: command output exceeds supported limit\n", stderr);
      close(pipefd[0]);
      (void)wait_child(pid);
      free(output->data);
      memset(output, 0, sizeof(*output));
      return -1;
    }
  }
  close(pipefd[0]);
  status = wait_child(pid);
  if (!output->data)
  {
    output->data = calloc(1, 1);
    if (!output->data)
      return -1;
  }
  return status;
}

static int
run_command(char *const argv[])
{
  pid_t pid = fork();

  if (pid < 0)
  {
    fprintf(stderr, "rawwin: fork: %s\n", strerror(errno));
    return -1;
  }
  if (pid == 0)
  {
    execvp(argv[0], argv);
    fprintf(stderr, "rawwin: cannot execute %s: %s\n", argv[0], strerror(errno));
    _exit(errno == ENOENT ? 127 : 126);
  }
  return wait_child(pid);
}

static int
hex_value(unsigned char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static char *
display_text(const char *text)
{
  size_t i, length = strlen(text), used = 0;
  char *result = malloc(length + 1u);

  if (!result)
    return NULL;
  for (i = 0; i < length; ++i)
  {
    unsigned char c = (unsigned char)text[i];
    if (c == '%' && i + 2u < length)
    {
      int hi = hex_value((unsigned char)text[i + 1u]);
      int lo = hex_value((unsigned char)text[i + 2u]);
      if (hi >= 0 && lo >= 0)
      {
        c = (unsigned char)((hi << 4) | lo);
        i += 2u;
      }
    }
    if (c < 0x20u || c == 0x7fu)
      c = ' ';
    result[used++] = (char)c;
  }
  result[used] = '\0';
  return result;
}

static int
build_menu(const char *snapshot, FILE *menu, size_t *client_count)
{
  char *copy, *line, *next;
  int first = 1;
  size_t count_clients = 0;

  *client_count = 0;
  copy = strdup(snapshot ? snapshot : "");
  if (!copy)
    return -1;
  line = copy;
  while (line)
  {
    char *endline = strchr(line, '\n');
    if (endline) { *endline = '\0'; next = endline + 1; }
    else next = NULL;

    if (first)
    {
      if (strcmp(line, "rawm-clients-v1"))
      {
        fputs("rawwin: unsupported or malformed rawm client inventory\n", stderr);
        free(copy);
        return -1;
      }
      first = 0;
    }
    else if (*line)
    {
      char *fields[8];
      size_t fields_count = 0;
      char *p = line;
      char *class_name, *title;

      fields[fields_count++] = p;
      while (*p)
      {
        if (*p == '\t')
        {
          *p = '\0';
          if (fields_count >= sizeof(fields) / sizeof(fields[0]))
          {
            free(copy);
            return -1;
          }
          fields[fields_count++] = p + 1;
        }
        ++p;
      }
      if (fields_count != 8 || strcmp(fields[0], "client"))
      {
        fputs("rawwin: malformed rawm client inventory\n", stderr);
        free(copy);
        return -1;
      }
      class_name = display_text(fields[6]);
      title = display_text(fields[7]);
      if (!class_name || !title)
      {
        free(class_name);
        free(title);
        free(copy);
        return -1;
      }
      if (fprintf(menu, "%s  %s - %s\n", fields[1], class_name, title) < 0)
      {
        free(class_name);
        free(title);
        free(copy);
        return -1;
      }
      free(class_name);
      free(title);
      ++count_clients;
    }
    line = next;
  }
  free(copy);
  if (first || fflush(menu) != 0)
    return -1;
  *client_count = count_clients;
  return 0;
}

static int
selection_xid(const char *selection, char *xid, size_t capacity)
{
  const char *p = selection;
  size_t length = 0;
  char *end = NULL;
  unsigned long value;

  while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
  {
    if (length + 1u >= capacity)
      return -1;
    xid[length++] = *p++;
  }
  xid[length] = '\0';
  if (!length)
    return -1;
  errno = 0;
  value = strtoul(xid, &end, 0);
  if (errno || !end || end == xid || *end || value == 0 || value > UINT32_MAX)
    return -1;
  return 0;
}

int
main(int argc, char **argv)
{
  char *clients_argv[] = {"rawctl", "clients", NULL};
  char *menu_argv[] = {"rawmenu", "-M", "select", "-p", "Window:", NULL};
  Buffer clients = {0}, selection = {0};
  FILE *menu = NULL;
  size_t client_count = 0;
  char xid[32];
  char *focus_argv[] = {"rawctl", "focus-client", xid, NULL};
  int status;

  if (argc == 2 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")))
  {
    puts(VERSION);
    return EXIT_SUCCESS;
  }
  if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
  {
    usage(stdout);
    return EXIT_SUCCESS;
  }
  if (argc != 1)
  {
    usage(stderr);
    return EXIT_FAILURE;
  }

  status = capture_command(clients_argv, NULL, &clients);
  if (status != 0)
  {
    free(clients.data);
    return EXIT_FAILURE;
  }
  menu = tmpfile();
  if (!menu)
  {
    fprintf(stderr, "rawwin: tmpfile: %s\n", strerror(errno));
    free(clients.data);
    return EXIT_FAILURE;
  }
  if (build_menu(clients.data, menu, &client_count) < 0)
  {
    fclose(menu);
    free(clients.data);
    return EXIT_FAILURE;
  }
  free(clients.data);
  if (!client_count)
  {
    fputs("rawwin: no admitted rawm clients\n", stderr);
    fclose(menu);
    return EXIT_FAILURE;
  }
  if (fseek(menu, 0, SEEK_SET) < 0)
  {
    fclose(menu);
    return EXIT_FAILURE;
  }

  status = capture_command(menu_argv, menu, &selection);
  fclose(menu);
  if (status != 0)
  {
    free(selection.data);
    return EXIT_FAILURE;
  }
  if (selection_xid(selection.data, xid, sizeof(xid)) < 0)
  {
    fputs("rawwin: rawmenu returned an invalid client selection\n", stderr);
    free(selection.data);
    return EXIT_FAILURE;
  }
  free(selection.data);

  return run_command(focus_argv) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

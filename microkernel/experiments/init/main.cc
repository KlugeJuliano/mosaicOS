#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <l4/ned/cmd_control>
#include <l4/re/env>
#include <l4/sys/cxx/ipc_string>

#define MAX_FIELD 128

struct service_manifest {
  char name[MAX_FIELD];
  char binary[MAX_FIELD];
  char restart[MAX_FIELD];
};

static char *trim(char *value)
{
  char *end;

  while (isspace((unsigned char)*value))
    value++;

  if (*value == '\0')
    return value;

  end = value + strlen(value) - 1;
  while (end > value && isspace((unsigned char)*end))
    *end-- = '\0';

  return value;
}

static int copy_field(char *dst, size_t dst_size, const char *src)
{
  size_t len = strlen(src);

  if (len >= dst_size)
    return -1;

  memcpy(dst, src, len + 1);
  return 0;
}

static int is_safe_lua_string(const char *value)
{
  for (; *value != '\0'; value++) {
    if (!(isalnum((unsigned char)*value)
          || *value == '-' || *value == '_' || *value == '/' || *value == '.'))
      return 0;
  }

  return 1;
}

static int parse_manifest(FILE *file, struct service_manifest *manifest)
{
  char line[256];
  unsigned line_no = 0;
  int in_services = 0;
  int in_first_service = 0;

  memset(manifest, 0, sizeof(*manifest));
  copy_field(manifest->restart, sizeof(manifest->restart), "never");

  while (fgets(line, sizeof(line), file) != NULL) {
    size_t indent = 0;
    char *key;
    char *value;
    char *separator = NULL;

    line_no++;
    while (line[indent] == ' ')
      indent++;

    key = trim(line);

    if (*key == '\0' || *key == '#')
      continue;

    separator = strchr(key, ':');
    if (separator == NULL) {
      printf("mosaic-init: manifest line %u is missing ':'\n", line_no);
      return -1;
    }

    *separator = '\0';
    value = trim(separator + 1);
    key = trim(key);

    if (indent == 0 && strcmp(key, "services") == 0) {
      in_services = 1;
      continue;
    }

    if (!in_services)
      continue;

    if (indent == 2 && value[0] == '\0') {
      if (manifest->name[0] != '\0') {
        printf("mosaic-init: only one service is supported in this milestone\n");
        return -1;
      }

      if (copy_field(manifest->name, sizeof(manifest->name), key) != 0) {
        printf("mosaic-init: service name is too long\n");
        return -1;
      }

      in_first_service = 1;
      continue;
    }

    if (!in_first_service || indent != 4) {
      printf("mosaic-init: unsupported manifest structure on line %u\n", line_no);
      return -1;
    }

    if (strcmp(key, "binary") == 0) {
      if (copy_field(manifest->binary, sizeof(manifest->binary), value) != 0) {
        printf("mosaic-init: service binary path is too long\n");
        return -1;
      }
    } else if (strcmp(key, "restart") == 0) {
      if (copy_field(manifest->restart, sizeof(manifest->restart), value) != 0) {
        printf("mosaic-init: restart policy is too long\n");
        return -1;
      }
    } else {
      printf("mosaic-init: unknown manifest key '%s' on line %u\n", key, line_no);
      return -1;
    }
  }

  if (manifest->name[0] == '\0') {
    printf("mosaic-init: manifest is missing service name\n");
    return -1;
  }

  if (manifest->binary[0] == '\0') {
    printf("mosaic-init: manifest is missing service binary\n");
    return -1;
  }

  if (!is_safe_lua_string(manifest->name) || !is_safe_lua_string(manifest->binary)) {
    printf("mosaic-init: manifest contains unsupported characters\n");
    return -1;
  }

  return 0;
}

static int start_service(const struct service_manifest *manifest)
{
  char command[512];
  long result;
  L4::Cap<L4Re::Ned::Cmd_control> ned;

  snprintf(command, sizeof(command),
           "local L4 = require('L4'); "
           "L4.default_loader:start({ log = { '%s', 'green' } }, '%s')",
           manifest->name, manifest->binary);

  ned = L4Re::Env::env()->get_cap<L4Re::Ned::Cmd_control>("svr");
  if (!ned.is_valid()) {
    printf("mosaic-init: missing Ned command capability 'svr'\n");
    return -1;
  }

  result = ned->execute(L4::Ipc::String<>(command));
  if (result < 0) {
    printf("mosaic-init: Ned command failed: %ld\n", result);
    return -1;
  }

  return 0;
}

int main(int argc, char **argv)
{
  const char *manifest_path = argc > 1 ? argv[1] : "rom/mosaic-init.manifest";
  struct service_manifest manifest;
  FILE *file;

  printf("mosaic-init: starting\n");
  printf("mosaic-init: loading manifest %s\n", manifest_path);

  file = fopen(manifest_path, "r");
  if (file == NULL) {
    printf("mosaic-init: failed to open manifest\n");
    return 1;
  }

  if (parse_manifest(file, &manifest) != 0) {
    fclose(file);
    return 1;
  }

  fclose(file);

  printf("mosaic-init: service '%s'\n", manifest.name);
  printf("mosaic-init: binary '%s'\n", manifest.binary);
  printf("mosaic-init: restart policy '%s'\n", manifest.restart);
  printf("mosaic-init: starting service '%s'\n", manifest.name);

  if (start_service(&manifest) != 0)
    return 1;

  printf("mosaic-init: service '%s' started\n", manifest.name);
  return 0;
}

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <l4/ned/cmd_control>
#include <l4/re/env>
#include <l4/sys/cxx/ipc_string>

#define MAX_FIELD 128
#define MAX_SERVICES 8
#define MAX_REQUIRES 4

enum service_state {
  STATE_DEFINED,
  STATE_STARTING,
  STATE_RUNNING,
  STATE_FAILED,
};

struct service_manifest {
  char name[MAX_FIELD];
  char binary[MAX_FIELD];
  char restart[MAX_FIELD];
  char dependencies[MAX_REQUIRES][MAX_FIELD];
  unsigned require_count;
  enum service_state state;
};

struct system_manifest {
  struct service_manifest services[MAX_SERVICES];
  unsigned service_count;
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
  if (*value == '\0')
    return 0;

  for (; *value != '\0'; value++) {
    if (!(isalnum((unsigned char)*value)
          || *value == '-' || *value == '_' || *value == '/' || *value == '.'))
      return 0;
  }

  return 1;
}

static const char *state_name(enum service_state state)
{
  switch (state) {
  case STATE_DEFINED:
    return "defined";
  case STATE_STARTING:
    return "starting";
  case STATE_RUNNING:
    return "running";
  case STATE_FAILED:
    return "failed";
  }

  return "unknown";
}

static int find_service(const struct system_manifest *manifest, const char *name)
{
  for (unsigned i = 0; i < manifest->service_count; i++) {
    if (strcmp(manifest->services[i].name, name) == 0)
      return (int)i;
  }

  return -1;
}

static int valid_restart_policy(const char *restart)
{
  return strcmp(restart, "never") == 0
         || strcmp(restart, "on-failure") == 0
         || strcmp(restart, "always") == 0;
}

static int add_service(struct system_manifest *manifest, const char *name)
{
  struct service_manifest *service;

  if (manifest->service_count >= MAX_SERVICES) {
    printf("mosaic-init: too many services; max is %u\n", MAX_SERVICES);
    return -1;
  }

  if (find_service(manifest, name) >= 0) {
    printf("mosaic-init: duplicate service '%s'\n", name);
    return -1;
  }

  service = &manifest->services[manifest->service_count];
  memset(service, 0, sizeof(*service));
  if (copy_field(service->name, sizeof(service->name), name) != 0) {
    printf("mosaic-init: service name is too long\n");
    return -1;
  }

  copy_field(service->restart, sizeof(service->restart), "never");
  service->state = STATE_DEFINED;
  manifest->service_count++;
  return (int)(manifest->service_count - 1);
}

static int add_requirement(struct service_manifest *service, const char *name)
{
  if (strcmp(service->name, name) == 0) {
    printf("mosaic-init: service '%s' cannot require itself\n", service->name);
    return -1;
  }

  for (unsigned i = 0; i < service->require_count; i++) {
    if (strcmp(service->dependencies[i], name) == 0) {
      printf("mosaic-init: service '%s' has duplicate dependency '%s'\n",
             service->name, name);
      return -1;
    }
  }

  if (service->require_count >= MAX_REQUIRES) {
    printf("mosaic-init: service '%s' has too many requirements\n", service->name);
    return -1;
  }

  if (copy_field(service->dependencies[service->require_count],
                 sizeof(service->dependencies[service->require_count]), name) != 0) {
    printf("mosaic-init: dependency name is too long\n");
    return -1;
  }

  service->require_count++;
  return 0;
}

static int visit_dependencies(const struct system_manifest *manifest,
                              unsigned service_index,
                              int *visiting,
                              int *visited)
{
  const struct service_manifest *service = &manifest->services[service_index];

  if (visiting[service_index]) {
    printf("mosaic-init: dependency cycle includes service '%s'\n", service->name);
    return -1;
  }

  if (visited[service_index])
    return 0;

  visiting[service_index] = 1;

  for (unsigned i = 0; i < service->require_count; i++) {
    int dependency = find_service(manifest, service->dependencies[i]);

    if (dependency < 0)
      return -1;

    if (visit_dependencies(manifest, (unsigned)dependency, visiting, visited) != 0)
      return -1;
  }

  visiting[service_index] = 0;
  visited[service_index] = 1;
  return 0;
}

static int validate_dependency_cycles(const struct system_manifest *manifest)
{
  int visiting[MAX_SERVICES];
  int visited[MAX_SERVICES];

  memset(visiting, 0, sizeof(visiting));
  memset(visited, 0, sizeof(visited));

  for (unsigned i = 0; i < manifest->service_count; i++) {
    if (visit_dependencies(manifest, i, visiting, visited) != 0)
      return -1;
  }

  return 0;
}

static int parse_manifest(FILE *file, struct system_manifest *manifest)
{
  char line[256];
  unsigned line_no = 0;
  int in_services = 0;
  int current = -1;
  int in_requires = 0;

  memset(manifest, 0, sizeof(*manifest));

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

    if (in_requires && indent == 6 && key[0] == '-') {
      char *required = trim(key + 1);

      if (*required == '\0') {
        printf("mosaic-init: empty dependency on line %u\n", line_no);
        return -1;
      }

      if (add_requirement(&manifest->services[current], required) != 0)
        return -1;

      continue;
    }

    in_requires = 0;
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
      current = -1;
      continue;
    }

    if (!in_services)
      continue;

    if (indent == 2 && value[0] == '\0') {
      current = add_service(manifest, key);
      if (current < 0)
        return -1;

      continue;
    }

    if (current < 0 || indent != 4) {
      printf("mosaic-init: unsupported manifest structure on line %u\n", line_no);
      return -1;
    }

    if (strcmp(key, "binary") == 0) {
      if (copy_field(manifest->services[current].binary,
                     sizeof(manifest->services[current].binary), value) != 0) {
        printf("mosaic-init: service binary path is too long\n");
        return -1;
      }
    } else if (strcmp(key, "restart") == 0) {
      if (copy_field(manifest->services[current].restart,
                     sizeof(manifest->services[current].restart), value) != 0) {
        printf("mosaic-init: restart policy is too long\n");
        return -1;
      }
    } else if (strcmp(key, "requires") == 0 && value[0] == '\0') {
      in_requires = 1;
    } else {
      printf("mosaic-init: unknown manifest key '%s' on line %u\n", key, line_no);
      return -1;
    }
  }

  if (manifest->service_count == 0) {
    printf("mosaic-init: manifest has no services\n");
    return -1;
  }

  for (unsigned i = 0; i < manifest->service_count; i++) {
    struct service_manifest *service = &manifest->services[i];

    if (service->binary[0] == '\0') {
      printf("mosaic-init: service '%s' is missing binary\n", service->name);
      return -1;
    }

    if (!is_safe_lua_string(service->name) || !is_safe_lua_string(service->binary)) {
      printf("mosaic-init: service '%s' contains unsupported characters\n", service->name);
      return -1;
    }

    if (!valid_restart_policy(service->restart)) {
      printf("mosaic-init: service '%s' has invalid restart policy '%s'\n",
             service->name, service->restart);
      return -1;
    }

    for (unsigned j = 0; j < service->require_count; j++) {
      if (!is_safe_lua_string(service->dependencies[j])) {
        printf("mosaic-init: service '%s' has invalid dependency name\n", service->name);
        return -1;
      }

      if (find_service(manifest, service->dependencies[j]) < 0) {
        printf("mosaic-init: service '%s' requires unknown service '%s'\n",
               service->name, service->dependencies[j]);
        return -1;
      }
    }
  }

  if (validate_dependency_cycles(manifest) != 0)
    return -1;

  return 0;
}

static int dependencies_running(const struct system_manifest *manifest,
                                const struct service_manifest *service)
{
  for (unsigned i = 0; i < service->require_count; i++) {
    int dependency = find_service(manifest, service->dependencies[i]);

    if (dependency < 0 || manifest->services[dependency].state != STATE_RUNNING)
      return 0;
  }

  return 1;
}

static void print_manifest(const struct system_manifest *manifest)
{
  printf("mosaic-init: loaded %u service(s)\n", manifest->service_count);

  for (unsigned i = 0; i < manifest->service_count; i++) {
    const struct service_manifest *service = &manifest->services[i];

    printf("mosaic-init: service '%s' binary '%s' restart '%s'\n",
           service->name, service->binary, service->restart);

    for (unsigned j = 0; j < service->require_count; j++)
      printf("mosaic-init: service '%s' requires '%s'\n",
             service->name, service->dependencies[j]);
  }
}

static void print_status(const struct system_manifest *manifest)
{
  printf("mosaic-init: service status\n");
  for (unsigned i = 0; i < manifest->service_count; i++) {
    const struct service_manifest *service = &manifest->services[i];
    printf("mosaic-init: status %-12s %s\n", service->name, state_name(service->state));
  }
}

static int start_service(struct service_manifest *service)
{
  char command[512];
  long result;
  L4::Cap<L4Re::Ned::Cmd_control> ned;

  snprintf(command, sizeof(command),
           "local L4 = require('L4'); "
           "L4.default_loader:start({ log = { '%s', 'green' } }, '%s')",
           service->name, service->binary);

  ned = L4Re::Env::env()->get_cap<L4Re::Ned::Cmd_control>("svr");
  if (!ned.is_valid()) {
    printf("mosaic-init: missing Ned command capability 'svr'\n");
    service->state = STATE_FAILED;
    return -1;
  }

  service->state = STATE_STARTING;
  result = ned->execute(L4::Ipc::String<>(command));
  if (result < 0) {
    printf("mosaic-init: Ned command for '%s' failed: %ld\n", service->name, result);
    service->state = STATE_FAILED;
    return -1;
  }

  service->state = STATE_RUNNING;
  return 0;
}

static int start_services(struct system_manifest *manifest)
{
  unsigned running = 0;
  unsigned round = 1;

  while (running < manifest->service_count) {
    int ready[MAX_SERVICES];
    unsigned started_this_round = 0;

    printf("mosaic-init: startup round %u\n", round);

    for (unsigned i = 0; i < manifest->service_count; i++) {
      const struct service_manifest *service = &manifest->services[i];

      ready[i] = service->state == STATE_DEFINED && dependencies_running(manifest, service);
    }

    for (unsigned i = 0; i < manifest->service_count; i++) {
      struct service_manifest *service = &manifest->services[i];

      if (!ready[i])
        continue;

      printf("mosaic-init: starting service '%s'\n", service->name);
      if (start_service(service) != 0)
        return -1;

      printf("mosaic-init: service '%s' started\n", service->name);
      started_this_round++;
      running++;
    }

    print_status(manifest);

    if (started_this_round == 0) {
      printf("mosaic-init: dependency resolution stalled\n");
      return -1;
    }

    round++;
  }

  return 0;
}

int main(int argc, char **argv)
{
  const char *manifest_path = argc > 1 ? argv[1] : "rom/mosaic-init.manifest";
  struct system_manifest manifest;
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

  print_manifest(&manifest);
  print_status(&manifest);

  if (start_services(&manifest) != 0) {
    print_status(&manifest);
    return 1;
  }

  printf("mosaic-init: all services started\n");
  print_status(&manifest);
  return 0;
}

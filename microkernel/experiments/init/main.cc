#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <l4/ned/cmd_control>
#include <l4/re/env>
#include <l4/sys/cxx/ipc_string>

#define MAX_FIELD 128
#define MAX_SERVICES 8
#define MAX_REQUIRES 4
#define MAX_CAPABILITIES 4

enum service_state {
  STATE_DEFINED,
  STATE_STARTING,
  STATE_RUNNING,
  STATE_RESTARTING,
  STATE_FAILED,
};

struct service_manifest {
  char name[MAX_FIELD];
  char binary[MAX_FIELD];
  char restart[MAX_FIELD];
  char start[MAX_FIELD];
  char fallback[MAX_FIELD];
  char dependencies[MAX_REQUIRES][MAX_FIELD];
  char capabilities[MAX_CAPABILITIES][MAX_FIELD];
  unsigned require_count;
  unsigned capability_count;
  unsigned max_restarts;
  unsigned restart_count;
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
  case STATE_RESTARTING:
    return "restarting";
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

static int valid_start_policy(const char *start)
{
  return strcmp(start, "auto") == 0 || strcmp(start, "manual") == 0;
}

static int service_needs_exit_monitor(const struct service_manifest *service)
{
  return strcmp(service->restart, "never") != 0 || service->max_restarts > 0;
}

static int parse_unsigned(const char *value, unsigned *result)
{
  char *end;
  unsigned long parsed;

  if (*value == '\0')
    return -1;

  parsed = strtoul(value, &end, 10);
  if (*end != '\0' || parsed > 1000)
    return -1;

  *result = (unsigned)parsed;
  return 0;
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
  copy_field(service->start, sizeof(service->start), "auto");
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

static int add_capability(struct service_manifest *service, const char *name)
{
  for (unsigned i = 0; i < service->capability_count; i++) {
    if (strcmp(service->capabilities[i], name) == 0) {
      printf("mosaic-init: service '%s' has duplicate capability '%s'\n",
             service->name, name);
      return -1;
    }
  }

  if (service->capability_count >= MAX_CAPABILITIES) {
    printf("mosaic-init: service '%s' has too many capabilities\n", service->name);
    return -1;
  }

  if (copy_field(service->capabilities[service->capability_count],
                 sizeof(service->capabilities[service->capability_count]), name) != 0) {
    printf("mosaic-init: capability name is too long\n");
    return -1;
  }

  service->capability_count++;
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
  int in_capabilities = 0;
  int in_recovery = 0;

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

    if (in_capabilities && indent == 6 && key[0] == '-') {
      char *capability = trim(key + 1);

      if (*capability == '\0') {
        printf("mosaic-init: empty capability on line %u\n", line_no);
        return -1;
      }

      if (add_capability(&manifest->services[current], capability) != 0)
        return -1;

      continue;
    }

    in_requires = 0;
    in_capabilities = 0;
    if (in_recovery && indent != 6)
      in_recovery = 0;

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

    if (current < 0 || (indent != 4 && !(in_recovery && indent == 6))) {
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
    } else if (strcmp(key, "start") == 0) {
      if (copy_field(manifest->services[current].start,
                     sizeof(manifest->services[current].start), value) != 0) {
        printf("mosaic-init: start policy is too long\n");
        return -1;
      }
    } else if (strcmp(key, "requires") == 0 && value[0] == '\0') {
      in_requires = 1;
    } else if (strcmp(key, "capabilities") == 0 && value[0] == '\0') {
      in_capabilities = 1;
    } else if (strcmp(key, "recovery") == 0 && value[0] == '\0') {
      in_recovery = 1;
    } else if (in_recovery && strcmp(key, "max_restarts") == 0) {
      if (parse_unsigned(value, &manifest->services[current].max_restarts) != 0) {
        printf("mosaic-init: invalid max_restarts for service '%s'\n",
               manifest->services[current].name);
        return -1;
      }
    } else if (in_recovery && strcmp(key, "fallback") == 0) {
      if (strcmp(value, "null") != 0
          && copy_field(manifest->services[current].fallback,
                        sizeof(manifest->services[current].fallback), value) != 0) {
        printf("mosaic-init: fallback service name is too long\n");
        return -1;
      }
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

    if (!valid_start_policy(service->start)) {
      printf("mosaic-init: service '%s' has invalid start policy '%s'\n",
             service->name, service->start);
      return -1;
    }

    if (service->fallback[0] != '\0') {
      if (!is_safe_lua_string(service->fallback)) {
        printf("mosaic-init: service '%s' has invalid fallback name\n", service->name);
        return -1;
      }

      if (find_service(manifest, service->fallback) < 0) {
        printf("mosaic-init: service '%s' has unknown fallback '%s'\n",
               service->name, service->fallback);
        return -1;
      }
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

    for (unsigned j = 0; j < service->capability_count; j++) {
      if (!is_safe_lua_string(service->capabilities[j])) {
        printf("mosaic-init: service '%s' has invalid capability name\n", service->name);
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

    printf("mosaic-init: service '%s' binary '%s' restart '%s' start '%s'\n",
           service->name, service->binary, service->restart, service->start);

    if (service->max_restarts > 0)
      printf("mosaic-init: service '%s' max_restarts %u\n",
             service->name, service->max_restarts);

    if (service->fallback[0] != '\0')
      printf("mosaic-init: service '%s' fallback '%s'\n",
             service->name, service->fallback);

    for (unsigned j = 0; j < service->require_count; j++)
      printf("mosaic-init: service '%s' requires '%s'\n",
             service->name, service->dependencies[j]);

    for (unsigned j = 0; j < service->capability_count; j++)
      printf("mosaic-init: service '%s' capability '%s'\n",
             service->name, service->capabilities[j]);
  }
}

static void append_service_caps(char *command, size_t command_size,
                                const struct service_manifest *service)
{
  if (service->capability_count == 0)
    return;

  strncat(command, ", caps = { ", command_size - strlen(command) - 1);
  for (unsigned i = 0; i < service->capability_count; i++) {
    if (i > 0)
      strncat(command, ", ", command_size - strlen(command) - 1);

    strncat(command, "['", command_size - strlen(command) - 1);
    strncat(command, service->capabilities[i], command_size - strlen(command) - 1);
    strncat(command, "'] = L4.Env['", command_size - strlen(command) - 1);
    strncat(command, service->capabilities[i], command_size - strlen(command) - 1);
    strncat(command, "']", command_size - strlen(command) - 1);
  }
  strncat(command, " }", command_size - strlen(command) - 1);
}

static void print_status(const struct system_manifest *manifest)
{
  printf("mosaic-init: service status\n");
  for (unsigned i = 0; i < manifest->service_count; i++) {
    const struct service_manifest *service = &manifest->services[i];
    printf("mosaic-init: status %-12s %s\n", service->name, state_name(service->state));
  }
}

static int has_failed_services(const struct system_manifest *manifest)
{
  for (unsigned i = 0; i < manifest->service_count; i++) {
    if (manifest->services[i].state == STATE_FAILED)
      return 1;
  }

  return 0;
}

static int has_defined_services(const struct system_manifest *manifest)
{
  for (unsigned i = 0; i < manifest->service_count; i++) {
    if (manifest->services[i].state == STATE_DEFINED
        && strcmp(manifest->services[i].start, "manual") != 0)
      return 1;
  }

  return 0;
}

static int start_service(struct service_manifest *service)
{
  char command[512];
  char options[256];
  long result;
  L4::Cap<L4Re::Ned::Cmd_control> ned;

  snprintf(options, sizeof(options), "{ log = { '%s', 'green' }", service->name);
  append_service_caps(options, sizeof(options), service);
  strncat(options, " }", sizeof(options) - strlen(options) - 1);

  if (service_needs_exit_monitor(service)) {
    snprintf(command, sizeof(command),
             "local L4 = require('L4'); "
             "local t = L4.default_loader:start(%s, '%s'); "
             "local rc = t:wait(); "
             "if rc ~= 0 then error('service %s exited with ' .. rc) end",
             options, service->binary, service->name);
  } else {
    snprintf(command, sizeof(command),
             "local L4 = require('L4'); "
             "L4.default_loader:start(%s, '%s')",
             options, service->binary);
  }

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

static int recover_service(struct system_manifest *manifest,
                           struct service_manifest *service)
{
  unsigned attempts = 0;
  unsigned max_attempts = service->max_restarts + 1;

  while (attempts < max_attempts) {
    attempts++;

    if (attempts > 1) {
      service->state = STATE_RESTARTING;
      service->restart_count++;
      printf("[CRASH] service=%s reason=TaskExit restart_count=%u action=restart\n",
             service->name, service->restart_count);
      printf("mosaic-init: restarting service '%s' (%u/%u)\n",
             service->name, service->restart_count, service->max_restarts);
    }

    if (start_service(service) == 0)
      return 0;

    printf("mosaic-init: service '%s' failed during attempt %u\n",
           service->name, attempts);

    if (strcmp(service->restart, "never") == 0)
      break;
  }

  service->state = STATE_FAILED;
  printf("[CRASH] service=%s reason=TaskExit restart_count=%u action=failed\n",
         service->name, service->restart_count);
  printf("mosaic-init: service '%s' marked failed after %u attempt(s)\n",
         service->name, attempts);

  if (service->fallback[0] != '\0') {
    int fallback_index = find_service(manifest, service->fallback);

    printf("mosaic-init: fallback '%s' is declared for service '%s'\n",
           service->fallback, service->name);

    if (fallback_index < 0) {
      printf("mosaic-init: fallback '%s' is not defined\n", service->fallback);
      return 0;
    }

    struct service_manifest *fallback = &manifest->services[fallback_index];
    if (fallback->state == STATE_RUNNING) {
      printf("mosaic-init: fallback '%s' already running\n", fallback->name);
      return 0;
    }

    if (fallback->state != STATE_DEFINED) {
      printf("mosaic-init: fallback '%s' is %s and cannot be started\n",
             fallback->name, state_name(fallback->state));
      return 0;
    }

    if (!dependencies_running(manifest, fallback)) {
      printf("mosaic-init: fallback '%s' dependencies are not running\n",
             fallback->name);
      return 0;
    }

    printf("[CRASH] service=%s reason=TaskExit restart_count=%u action=fallback:%s\n",
           service->name, service->restart_count, fallback->name);
    printf("mosaic-init: starting fallback '%s' for service '%s'\n",
           fallback->name, service->name);

    if (recover_service(manifest, fallback) == 0 && fallback->state == STATE_RUNNING)
      printf("mosaic-init: fallback '%s' started\n", fallback->name);
  }

  return 0;
}

static int start_services(struct system_manifest *manifest)
{
  unsigned round = 1;

  while (has_defined_services(manifest)) {
    int ready[MAX_SERVICES];
    unsigned started_this_round = 0;

    printf("mosaic-init: startup round %u\n", round);

    for (unsigned i = 0; i < manifest->service_count; i++) {
      const struct service_manifest *service = &manifest->services[i];

      ready[i] = service->state == STATE_DEFINED
                 && strcmp(service->start, "manual") != 0
                 && dependencies_running(manifest, service);
    }

    for (unsigned i = 0; i < manifest->service_count; i++) {
      struct service_manifest *service = &manifest->services[i];

      if (!ready[i] || service->state != STATE_DEFINED)
        continue;

      printf("mosaic-init: starting service '%s'\n", service->name);
      if (recover_service(manifest, service) != 0)
        return -1;

      if (service->state == STATE_RUNNING)
        printf("mosaic-init: service '%s' started\n", service->name);
      started_this_round++;
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

  if (has_failed_services(&manifest))
    printf("mosaic-init: startup completed with failed services\n");
  else
    printf("mosaic-init: all services started\n");

  print_status(&manifest);
  return 0;
}

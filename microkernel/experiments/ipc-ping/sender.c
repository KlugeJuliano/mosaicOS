#include <stdio.h>
#include <l4/re/env.h>
#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>

int main(void)
{
  l4_cap_idx_t server = l4re_env_get_cap("ping_server");
  if (l4_is_invalid_cap(server))
    {
      printf("Sender: Could not find server capability!\n");
      return 1;
    }

  printf("Sender: Sending message to receiver...\n");

  l4_msgtag_t tag = l4_ipc_call(server, l4_utcb(), l4_msgtag(0, 0, 0, 0), L4_IPC_NEVER);

  if (l4_ipc_error(tag, l4_utcb()))
    {
      printf("Sender: IPC error!\n");
      return 1;
    }

  printf("Sender: Message sent and reply received.\n");
  return 0;
}

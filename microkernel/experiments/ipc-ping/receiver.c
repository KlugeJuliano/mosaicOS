#include <stdio.h>
#include <string.h>
#include <l4/re/env.h>
#include <l4/sys/ipc.h>
#include <l4/sys/rcv_endpoint.h>
#include <l4/sys/utcb.h>

int main(void)
{
  l4_cap_idx_t server = l4re_env_get_cap("ping_server");
  if (l4_is_invalid_cap(server))
    {
      printf("Receiver: Could not find server capability!\n");
      return 1;
    }

  l4_msgtag_t bind = l4_rcv_ep_bind_thread(server, l4re_env()->main_thread, 0x1000);
  if (l4_error(bind) < 0)
    {
      printf("Receiver: Could not bind server capability: %d\n", l4_error(bind));
      return 1;
    }

  printf("Receiver: Waiting for message...\n");

  l4_umword_t label;
  l4_msgtag_t tag = l4_ipc_wait(l4_utcb(), &label, L4_IPC_NEVER);

  while (1)
    {
      if (l4_ipc_error(tag, l4_utcb()))
        {
          printf("Receiver: IPC error!\n");
          return 1;
        }

      char message[sizeof(l4_umword_t) * 8 + 1];
      memcpy(message, l4_utcb_mr()->mr, sizeof(message) - 1);
      message[sizeof(message) - 1] = '\0';

      printf("Receiver: Received IPC from label %lx: %s\n", label, message);

      // Reply and wait for next
      tag = l4_ipc_reply_and_wait(l4_utcb(), l4_msgtag(0, 0, 0, 0), &label, L4_IPC_NEVER);
    }

  return 0;
}

#include <stdio.h>
#include <l4/re/env.h>
#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>

int main(void)
{
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

      printf("Receiver: Received IPC from label %lx\n", label);

      // Reply and wait for next
      tag = l4_ipc_reply_and_wait(l4_utcb(), l4_msgtag(0, 0, 0, 0), &label, L4_IPC_NEVER);
    }

  return 0;
}

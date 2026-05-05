#include <stdio.h>
#include <string.h>
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

  const char *message = "MosaicOS Lab: IPC ping";
  l4_msg_regs_t *mr = l4_utcb_mr();
  memset(mr->mr, 0, sizeof(mr->mr[0]) * 8);
  strncpy((char *)mr->mr, message, sizeof(mr->mr[0]) * 8 - 1);

  l4_msgtag_t tag = l4_ipc_call(server, l4_utcb(), l4_msgtag(0, 8, 0, 0), L4_IPC_NEVER);

  if (l4_ipc_error(tag, l4_utcb()))
    {
      printf("Sender: IPC error!\n");
      return 1;
    }

  printf("Sender: Message sent and reply received.\n");
  return 0;
}

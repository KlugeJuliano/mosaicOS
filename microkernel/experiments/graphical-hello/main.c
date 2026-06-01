#include <stdio.h>

int main(void)
{
  printf("MosaicOS Graphics: app mosaic-hello connected compositor=ready\n");
  printf("MosaicOS Graphics: app mosaic-hello create-window width=400 height=300 title='MosaicOS Hello'\n");
  printf("MosaicOS Graphics: app mosaic-hello draw background=#1a1a2e label='MosaicOS Lab'\n");
  printf("MosaicOS Graphics: app mosaic-hello input KeyDown key=Q action=exit\n");
  return 0;
}

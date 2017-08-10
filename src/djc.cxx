#include "DimDQMJobControl.h"
#include <unistd.h>
#include <stdint.h>

int main()
{
  DimDQMJobControl* c=new DimDQMJobControl();

  //s->scandns();
  //getchar();
  //s->scan();
  //getchar();
  //s->initialise();
  while (true)
    sleep((unsigned int) 3);
}

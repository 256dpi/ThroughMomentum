#include "smooth.h"

double smooth_update(smooth_t *s, double v) {
  // subtract last reading
  s->total -= s->values[s->index];

  // save reading
  s->values[s->index] = v;

  // add reading
  s->total += v;

  // increment index
  s->index++;

  // check overflow
  if (s->index >= 10) {
    s->index = 0;
  }

  // update cont
  if (s->count < 10) {
    s->count++;
  }

  // return average
  return s->total / (double)s->count;
}

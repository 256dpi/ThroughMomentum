typedef struct {
  double values[10];
  int index;
  double total;
  int count;
} smooth_t;

#define smooth_default {{0, 0, 0, 0, 0}, 0, 0, 0};

double smooth_update(smooth_t *s, double v);

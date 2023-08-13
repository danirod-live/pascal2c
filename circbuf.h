#pragma once

#include <stddef.h>

typedef struct circbuf circbuf_t;

circbuf_t *circbuf_init(void);

int circbuf_write(circbuf_t *, int);

int circbuf_read(circbuf_t *);

int circbuf_peek(circbuf_t *);

int circbuf_peekfar(circbuf_t *, unsigned int);

int circbuf_empty(circbuf_t *);

size_t circbuf_bufsiz(circbuf_t *);

void circbuf_free(circbuf_t *);

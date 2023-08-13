#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circbuf.h"

#define CIRCBUF_DEFAULT_SIZE 8

struct circbuf {
	int *buf;
	int siz;
	int write;
	int read;
};

static int
circbuf_normalize(circbuf_t *circ)
{
	int *tmpbuf;
	int new_write;

	if ((tmpbuf = malloc(sizeof(int) * circ->siz)) == 0) {
		return -1;
	}

	// reassemble the buffer so that the reading area starts at 0
	memcpy(tmpbuf,
	       circ->buf + circ->read,
	       sizeof(int) * (circ->siz - circ->read));
	memcpy(tmpbuf + circ->siz - circ->read,
	       circ->buf,
	       sizeof(int) * circ->read);

	// reposition write (given that read will be at 0).
	new_write = circ->write - circ->read;
	while (new_write < 0)
		new_write += circ->siz;
	while (new_write >= circ->siz)
		new_write -= circ->siz;

	// update the circular buffer and cleanup
	memcpy(circ->buf, tmpbuf, sizeof(int) * circ->siz);
	circ->read = 0;
	circ->write = new_write;
	free(tmpbuf);
	return 0;
}

static int
circbuf_resize(circbuf_t *circ)
{
	int newsiz;
	int *newbuf;

	// set the initial size for the new copy of the buffer
	if (circ->siz == 0) {
		newsiz = CIRCBUF_DEFAULT_SIZE;
	} else {
		newsiz = circ->siz * 2;
	}
	if ((newbuf = malloc(sizeof(int) * newsiz)) == 0) {
		return -1;
	}

	// copy the old buffer into the new if necessary
	if (circ->buf) {
		if (circbuf_normalize(circ) == -1) {
			return -1;
		}
		memcpy(newbuf, circ->buf, sizeof(int) * circ->siz);
		free(circ->buf);
	}

	// update the buffer coordinates.
	circ->siz = newsiz;
	circ->buf = newbuf;
	return 0;
}

circbuf_t *
circbuf_init(void)
{
	circbuf_t *circ = malloc(sizeof(circbuf_t));
	if (circ != 0) {
		circ->buf = NULL;
		circ->siz = 0;
		circ->write = 0;
		circ->read = 0;
		circbuf_resize(circ);
		memset(circ->buf, 0, sizeof(int) * circ->siz);
	}
	return circ;
}

int
circbuf_write(circbuf_t *circ, int ch)
{
	int oldsiz;

	circ->buf[circ->write++] = ch;
	if (circ->write == circ->siz) {
		circ->write = 0;
	}

	// the buffer cannot hold anymore, time to scale
	if (circ->read == circ->write) {
		oldsiz = circ->siz;
		if (circbuf_resize(circ) < 0) {
			return -1;
		}
		circ->write = oldsiz;
	}
	return 0;
}

int
circbuf_read(circbuf_t *circ)
{
	int ch;
	if (circ->read == circ->write) {
		return -1;
	}

	ch = circ->buf[circ->read++];
	if (circ->read == circ->siz) {
		circ->read = 0;
	}
	return ch;
}

int
circbuf_peek(circbuf_t *circ)
{
	if (circ->read == circ->write) {
		return -1;
	}
	return circ->buf[circ->read];
}

int
circbuf_peekfar(circbuf_t *circ, unsigned int off)
{
	int target;

	// no content in the buffer, cannot read anyway.
	if (circbuf_empty(circ)) {
		return -1;
	}

	// read the target position and clamp it
	target = circ->read + off;
	while (target >= circ->siz)
		target -= circ->siz;

	// the unreadable area depends on the status of the buffer
	// (to readers: if you paint this, you will understand)
	if (circ->write > circ->read) {
		if (target >= circ->read && target < circ->write) {
			return circ->buf[target];
		}
	} else {
		if (target >= circ->read || target < circ->write) {
			return circ->buf[target];
		}
	}
	return -1;
}

int
circbuf_empty(circbuf_t *circ)
{
	return (circ->read == circ->write);
}

size_t
circbuf_bufsiz(circbuf_t *circ)
{
	if (circ->write < circ->read) {
		return (circ->write + circ->siz) - circ->read;
	}
	return circ->write - circ->read;
}

void
circbuf_free(circbuf_t *circ)
{
	if (circ->buf) {
		free(circ->buf);
	}
}

void
circbuf_debug(circbuf_t *circ)
{
	puts("DEBUG");
	puts("=====");
	for (int i = 0; i < circ->siz; i++) {
		fputc(i == circ->read ? 'R' : ' ', stdout);
	}
	puts("|");

	for (int i = 0; i < circ->siz; i++) {
		fputc(circ->buf[i] < 0x20 ? ' ' : circ->buf[i], stdout);
	}
	puts("|");

	for (int i = 0; i < circ->siz; i++) {
		fputc(i == circ->write ? 'W' : ' ', stdout);
	}
	puts("|");
	puts("========");
	puts("ENDDEBUG");
	puts("========");
}

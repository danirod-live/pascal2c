#pragma once

#include "token.h"
#include <stdio.h>
typedef struct scanner scanner_t;

scanner_t *scanner_init(FILE *);

tokentype_t scanner_next(scanner_t *);

void scanner_free(scanner_t *);

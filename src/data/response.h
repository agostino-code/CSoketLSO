#ifndef RESPONSE_H
#define RESPONSE_H

#include <json.h>
typedef struct {
    const char* type;
    json_object* data;
} Response;

#endif

#ifndef REQUEST_H
#define REQUEST_H

#include <json.h>
typedef struct {
    const char* type;
    json_object* data;
} Request;

#endif // REQUEST_H

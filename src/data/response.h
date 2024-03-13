#ifndef RESPONSE_H
#define RESPONSE_H

// Add your code here

//Request struct

#include <json.h>
typedef struct {
    const char* type;
    json_object* data;
} Response;

#endif

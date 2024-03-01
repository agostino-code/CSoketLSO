#ifndef REQUEST_H
#define REQUEST_H

// Add your code here

//Request struct

#include <json.h>
typedef struct {
    const char* type;
    json_object* data;
} Request;

#endif // REQUEST_H

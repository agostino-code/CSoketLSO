#ifndef REQUEST_H
#define REQUEST_H

// Add your code here

//Request struct

#include <yyjson.h>
typedef struct {
    const char* type;
    yyjson_val* data;
} Request;

#endif // REQUEST_H

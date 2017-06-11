#ifndef HEX_H
#define HEX_H

struct output {
    int *digit;
    int length;
};

struct output *GetASMValue(unsigned long number);

#endif
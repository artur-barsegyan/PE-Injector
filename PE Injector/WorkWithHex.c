#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "WorkWithHex.h"

#define BITINBYTE 8
#define RELEASE

static void InsertBit(unsigned char *byte, int pos, int value) {
    unsigned char multiplier = (unsigned char)pow(2, pos);

    if (value)
        *byte = *byte | multiplier;
}

static struct output *GetDecFromBinary(struct output *BinNumber) {
    int counter = 0;
    int length = BinNumber->length;
    int newlength = 0;
    int *decoutput = (int*)calloc(32, sizeof(int));

    while (counter < length) {
        if (length - counter >= BITINBYTE) {
            int i = 0;
            unsigned char byte = 0;

            for (; i < 8; i++)
                InsertBit(&byte, i, BinNumber->digit[i + counter]);
    
            decoutput[newlength++] = byte;
            counter += 8;
        } else {
            int remainderbits = length - counter;

            int i = 0;
            unsigned char byte = 0;

            for (; i < 8; i++)
                InsertBit(&byte, i, BinNumber->digit[i + counter]);

            counter += remainderbits;
            decoutput[newlength++] = byte;
        }
    }

    struct output *DecOutput = (struct output*)calloc(1, sizeof(struct output));
    DecOutput->digit = decoutput;
    DecOutput->length = newlength;

    return DecOutput;
}

static struct output *GetBinary(unsigned long number) {
    int remainder = 0;
    int counter = 0;
    int *remainders = (int*)calloc(32, sizeof(int));

    while (number) {
        remainder = number % 2;
        number /= 2;
        remainders[counter++] = remainder;
    }

    struct output *binary = (struct output*)calloc(1, sizeof(struct output));
    binary->digit = remainders;
    binary->length = counter;

    return binary;
}

struct output *GetASMValue(unsigned long number) {
    int i = 0;

    struct output *BinNumber = GetBinary(number);
#ifdef DEBUG
    for (; i < BinNumber->length; i++)
        printf("%d", BinNumber->digit[i]);
    printf("\n");
#endif

    struct output *DecOutput = GetDecFromBinary(BinNumber);
#ifdef DEBUG
    for (i = 0; i < DecOutput->length; i++)
        printf("%x", DecOutput->digit[i]);
    printf("\n");
#endif

    return DecOutput;
}
#ifndef CARD_H
#define CARD_H

#include <stdint.h>
#include <stddef.h>


typedef struct
{
    int num;
    int numberHead;
    uint32_t client_id;

}card_t;

#endif //CARD_H
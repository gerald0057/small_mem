#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "inc/smem.h"

void print_memory(const void *mem, size_t length)
{
    const unsigned char *byte = (const unsigned char *)mem;
    size_t i, j;

    for (i = 0; i < length; i += 16)
    {
        // Print the memory address
        printf("%08zx  ", i);

        // Print the hex values
        for (j = 0; j < 16; j++)
        {
            if (i + j < length)
            {
                printf("%02x ", byte[i + j]);
            }
            else
            {
                printf("   ");
            }
        }

        // Print the ASCII representation
        printf(" ");
        for (j = 0; j < 16; j++)
        {
            if (i + j < length)
            {
                unsigned char c = byte[i + j];
                printf("%c", isprint(c) ? c : '.');
            }
        }

        printf("\n");
    }
}

int main(void)
{
    printf("Hello small mem\r\n");

    int heap_size = 128;
    uint8_t *heap_addr = malloc(heap_size);
    assert(heap_addr);
    memset(heap_addr, 0, heap_size);

    printf("Small mem init done\r\n");

    smem_t heap = 0;
    heap = smem_init(heap, heap_size);
    print_memory(heap_addr, heap_size);
    
    uint8_t *bufa = (uint8_t *)smem_alloc(heap, 20);
    memset(bufa, 0x0a, 20);
    print_memory(heap_addr, heap_size);
    
    uint8_t *bufb = (uint8_t *)smem_alloc(heap, 24);
    memset(bufa, 0x0b, 24);
    print_memory(heap_addr, heap_size);
    
    smem_free(bufa);
    uint8_t *bufc = (uint8_t *)smem_alloc(heap, 18);
    memset(bufa, 0x0c, 18);
    print_memory(heap_addr, heap_size);

    (void)(bufa);
    (void)(bufb);
    (void)(bufc);

    return 0;
}

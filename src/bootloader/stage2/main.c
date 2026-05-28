#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t bootDrive)
{
    puts("dhokaOS bootloader (stage 2)\r\n");

    printf("Formatted %s: %% %c %d %i\r\n", "string", 'a', 1234, -5678);
    printf("16-bit hex: %x %x %x\r\n", 0xbeef, 0xcafe, 0x1234);
    printf("Octal: %o\r\n", 12345);
    printf("32-bit long: %ld %lx\r\n", -100000000l, 0xdeadbeefl);
    printf("Short: %hd %hx %hhu\r\n", (short)27, (short)0x1234, (unsigned char)'A');
    printf("Long long: %lld %llx\r\n", -10000000000ll, 0xdeadbeefcafebabell);

end:
    for (;;);
}

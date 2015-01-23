MEMORY
{
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 256K
	ram (rwx) : ORIGIN = 0x20000000 + 2K, LENGTH = 64K - 2K
}

INCLUDE libopencm3_stm32f4.ld



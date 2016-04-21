MEMORY
{
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
	ram (rwx) : ORIGIN = 0x20000000 + 2K, LENGTH = 128K - 2K
}

INCLUDE libopencm3_stm32f4.ld



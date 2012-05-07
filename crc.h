#ifndef CRC16_H
#define CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Derived from CRC algorithm for JTAG ICE mkII, published in Atmel
 * Appnote AVR067.  Converted from C++ to C.
 */

extern unsigned short crcsum(const unsigned char* message,
			     unsigned long length,
			     unsigned short crc);
/*
 * Verify that the last two bytes is a (LSB first) valid CRC of the
 * message.
 */
extern int crcverify(const unsigned char* message,
		     unsigned long length);
/*
 * Append a two byte CRC (LSB first) to message.  length is size of
 * message excluding crc.  Space for the CRC bytes must be allocated
 * in advance!
 */
extern void crcappend(unsigned char* message,
		      unsigned long length);

extern unsigned short crc_table[256];

/* CRC calculation macros */
#define CRC_INIT 0xFFFF
#define CRC(crcval,newchar) crcval = (crcval >> 8) ^ \
	crc_table[(crcval ^ newchar) & 0x00ff]

#ifdef __cplusplus
}
#endif

#endif
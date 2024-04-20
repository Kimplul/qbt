#ifndef ABI_H
#define ABI_H

#include <qbt/nodes.h>

enum {
	RX0,
	RX1,
	RX2,
	RX3,
	RX4,
	RX5,
	RX6,
	RX7,
	RX8,
	RX9,
	RX10,
	RX11,
	RX12,
	RX13,
	RX14,
	RX15,
	RX16,
	RX17,
	RX18,
	RX19,
	RX20,
	RX21,
	RX22,
	RX23,
	RX24,
	RX25,
	RX26,
	RX27,
	RX28,
	RX29,
	RX30,
	RX31,
	RX32,
	RX33,
	RX34,
	RX35,
	RX36,
	RX37,
	RX38,
	RX39,
	RX40,
	RX41,
	RX42,
	RX43,
	RX44,
	RX45,
	RX46,
	RX47,
	RX48,
	RX49,
	RX50,
	RX51,
	RX52,
	RX53,
	RX54,
	RX55,
	RX56,
	RX57,
	RX58,
	RX59,
	RX60,
	RX61,
	RX62,
	RX63,
	RX64,
	RX65,
	RX66,
	RX67,
	RX68,
	RX69,
	RX70,
	RX71,
	RX72,
	RX73,
	RX74,
	RX75,
	RX76,
	RX77,
	RX78,
	RX79,
	RX80,
};

enum {
	RSP = RX1,
	RFP,
	RGP,
	RA0,
	RA1,
	RA2,
	RA3,
	RA4,
	RA5,
	RA6,
	RT0,
	RT1,
	RT2,
	RT3,
	RT4,
	RT5,
	RT6,
	RS0,
	RS1,
	RS2,
	RS3,
	RS4,
	RS5,
	RS6,
	RTP,
	RRA,
	RA7,
	RA8,
	RA9,
	RA10,
	RA11,
	RA12,
	RA13,
	RA14,
	RA15,
	RA16,
	RA17,
	RA18,
	RA19,
	RA20,
	RA21,
	RA22,
	RA23,
	RTMP0,
	RT7,
	RT8,
	RT9,
	RT10,
	RT11,
	RT12,
	RT13,
	RT14,
	RT15,
	RT16,
	RT17,
	RT18,
	RT19,
	RT20,
	RT21,
	RT22,
	RT23,
	RTMP1,
	RS7,
	RS8,
	RS9,
	RS10,
	RS11,
	RS12,
	RS13,
	RS14,
	RS15,
	RS16,
	RS17,
	RS18,
	RS19,
	RS20,
	RS21,
	RS22,
	RS23,
	RTMP2,
};

#define FOREACH_REG(M) \
	M(RX0)         \
	M(RX1)         \
	M(RX2)         \
	M(RX3)         \
	M(RX4)         \
	M(RX5)         \
	M(RX6)         \
	M(RX7)         \
	M(RX8)         \
	M(RX9)         \
	M(RX10)        \
	M(RX11)        \
	M(RX12)        \
	M(RX13)        \
	M(RX14)        \
	M(RX15)        \
	M(RX16)        \
	M(RX17)        \
	M(RX18)        \
	M(RX19)        \
	M(RX20)        \
	M(RX21)        \
	M(RX22)        \
	M(RX23)        \
	M(RX24)        \
	M(RX25)        \
	M(RX26)        \
	M(RX27)        \
	M(RX28)        \
	M(RX29)        \
	M(RX30)        \
	M(RX31)        \
	M(RX32)        \
	M(RX33)        \
	M(RX34)        \
	M(RX35)        \
	M(RX36)        \
	M(RX37)        \
	M(RX38)        \
	M(RX39)        \
	M(RX40)        \
	M(RX41)        \
	M(RX42)        \
	M(RX43)        \
	M(RX44)        \
	M(RX45)        \
	M(RX46)        \
	M(RX47)        \
	M(RX48)        \
	M(RX49)        \
	M(RX50)        \
	M(RX51)        \
	M(RX52)        \
	M(RX53)        \
	M(RX54)        \
	M(RX55)        \
	M(RX56)        \
	M(RX57)        \
	M(RX58)        \
	M(RX59)        \
	M(RX60)        \
	M(RX61)        \
	M(RX62)        \
	M(RX63)        \
	M(RX64)        \
	M(RX65)        \
	M(RX66)        \
	M(RX67)        \
	M(RX68)        \
	M(RX69)        \
	M(RX70)        \
	M(RX71)        \
	M(RX72)        \
	M(RX73)        \
	M(RX74)        \
	M(RX75)        \
	M(RX76)        \
	M(RX77)        \
	M(RX78)        \
	M(RX79)        \
	M(RX80)

void abi0(struct fn *f);

#endif /* ABI_H */

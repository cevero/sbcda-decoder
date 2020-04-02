/**
 * Every symbol is a 10bit value in 2's complement format. However,
 * they are placed into a 16bit unsigned int with no sign extension.
 * This function sign extends the symbol values and then compare
 * them in order to determine if they correspond to a 1 or a 0 bit.
 * The following convention is assumed:
 *
 * if symb0 < symb1 => (-1) to (1) transition => bit 0
 * if symb0 >= symb1 =>  (1) to (-1) transition => bit 1
 *
 */
unsigned int decodeBit(unsigned int symb0, unsigned int symb1)
{
	int stmp0 = 0, stmp1 = 0;

	// test the 10th bit and sign extends to 16 bit signed int (sign extend)
	stmp0 = symb0 & 0x0200 ? (0xFC00 | symb0) : (0x03FF & symb0);
	stmp1 = symb1 & 0x0200 ? (0xFC00 | symb1) : (0x03FF & symb1);

	if (stmp0 > stmp1) { // bit 1
		return 1;
	} else {
		return 0;
	}
}
/**
 * Returns the correspondent data portion size in bits
 * for a given message length code.
 * OBS: Assuming a 28bit ID_NUMBER format
 */
unsigned int calcMessageLength(unsigned int msgByteLength)
{

	switch (msgByteLength) {
	case 0:
		return 56 /*4MSG Length + 28 bits ID + 24 bits MSG */;
		break;
	case 3:
		return 90 /*4+28+56 bits*/;
		break;
	case 5:
		return 120 /*4+28+88 bits*/;
		break;
	case 6:
		return 152 /*4+28+120 bits*/;
		break;
	case 9:
		return 184 /*4+28+152 bits*/;
		break;
	case 10:
		return 216 /*4+28+184 bits*/;
		break;
	case 12:
		return 248 /*4+28+216 bits*/;
		break;
	case 15:
		return 280 /*4+28+248 bits*/;
		break;
	default:
		return 280 /*4+28+248 bits*/;
		break;
	}
}
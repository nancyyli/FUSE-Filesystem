
/*
    Run through the bits until a 0 is found.
    returns -1 if not 0 bit is found.
*/
/*int
get_bit_index(char* bits, int size)
{
    int count = 0;
    char temp = bits[0]; // maybe, might have to look into it.

    for (int i = 0; i < size; i++) {

        if (count >= 8) {
            count = 0;
            temp = bits[i + 1];
        }
        else {
            if (!(((temp >> count) & 1) ^ 0)) {
                return count + (i * 8);
            }
            i--;
        }
    }

    return -1;
}

/*
    sets the indecated bit to a certain value.
*/
/*
void
set_bit(char* bits, int size, int val, int index)
{
    // find the largest multiple of 8. this will lead to the required char.
    // then use the rest of index to find the specific bit to be set.
    //
    // (char that contains the bit) |= 1 << (rest of index)
    // does this just set a bit to one?

    int char_index = index / 8;
    int bit_index = index % 8;
    //index = first_part + second_part;

    char temp = bits[char_index];

    if (val) {
        // set bit
        temp |= 1 << bit_index;
    }
    else {
        // unset bit
        temp &= ~(1 << bit_index);
    }
}*/

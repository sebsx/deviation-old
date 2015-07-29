/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include <stdio.h>
#include <string.h>
/*
# File is Run-length encoded as follows:
## Read a u16 as length
## if length > 0, read length bytes into memory
## else if length == 0, read next byte as length, and fill length bytes with 0
## Repeat until EOF
*/
static void write_zeros(FILE *fh, int count)
{
    while(count--) {
        fputc(0, fh);
    }
}
static void write_data(FILE *fh, u8 *data, int count)
{
    fwrite(data, 1, count, fh);
}
void CONFIG_BinWrite(FILE *fh, void *data, int len)
{
    u8 *ptr = (u8 *)data;
    u8 *last_write = ptr;
    int count = 0;
    int zeros = 0;;
    const int min_zeros = 6;

    while (ptr < (u8*)data+len) {
        if (*ptr == 0 && zeros < 255) {
            zeros++;
        } else {
           if (zeros) {
               if (zeros >= min_zeros) {
                   write_data(fh, last_write, count);
                   write_zeros(fh, zeros);
                   last_write = ptr+1;
               } else {
                   count += zeros;
               }
               zeros = 0;
           }
           count++;
        }
        ptr++;
    }
    //No need to write zeros at end of file
    if (zeros < min_zeros) {
        count += zeros;
    }
    write_data(fh, last_write, count);
} 

void CONFIG_BinRead(FILE *fh, void *data, int length)
{
    u8 lenstr[3];
    u8 *ptr = (u8 *)data;
    while(ptr < (u8*)data + length) {
        if (fread(lenstr, 1, 3, fh) != 3) {
            break;
        }
        int len = lenstr[0] | (lenstr[1] << 8);
        if (len == 0) {
            len = lenstr[2];
            memset(ptr, 0, len);
            ptr += len;
        } else {
            *ptr = lenstr[2];
            len--;
            fread(ptr, 1, len, fh);
        }
    }
}
void CONFIG_BinReadFile(char *file, void *data, int length)
{
    FILE *fh = fopen(file, "w");
    if (! fh) {
        printf("Couldn't open %s\n", file);
        return;
    }
    CONFIG_BinRead(fh, data, length);
    fclose(fh);
}
void CONFIG_BinWriteFile(char *file, void *data, int length)
{
    FILE *fh = fopen(file, "r");
    if (! fh) {
        printf("Couldn't open %s\n", file);
        return;
    }
    CONFIG_BinWrite(fh, data, length);
    fclose(fh);
}


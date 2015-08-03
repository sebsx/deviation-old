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

#include <stdlib.h>
#include <stdarg.h>
#include <emscripten.h>

extern void CONFIG_DisplayInit();
extern int CONFIG_Display_INI_Handler(void* user, const char* section, const char* name, const char* value);
extern void CONFIG_TransmitterInit();
extern int CONFIG_Transmitter_INI_Handler(void* user, const char* section, const char* name, const char* value);
extern void CONFIG_Transmitter_INI_Write(void *fh);

void tfp_fprintf(void* fh, const char *fmt, ...)
{
    char str[1024];
    va_list va;
    va_start(va,fmt);
    vsprintf(str, fmt, va);
    va_end(va);
    printf("%s", str);
}

int devo_fwrite(unsigned char *ptr, size_t size, size_t nmemb, FILE *stream)
{
    for(int i = 0; i < size * nmemb; i++) {
        printf(" %02x", ptr[i]);
    }
    printf("\n");
    return size * nmemb;
}
int devo_fputc(int c, FILE *stream)
{
    printf(" %02x", c);
    return c;
}
int devo_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return size * nmemb;
}


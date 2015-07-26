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

extern void CONFIG_DisplayInit();
extern int CONFIG_Display_INI_Handler(void* user, const char* section, const char* name, const char* value);
int main(void) {
    CONFIG_DisplayInit();
    CONFIG_Display_INI_Handler(NULL, NULL, NULL, NULL);
    return 0;
}

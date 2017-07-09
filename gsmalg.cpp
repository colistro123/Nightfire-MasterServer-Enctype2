/*
GSMSALG 0.3.3
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org


LICENSE
=======
    Copyright 2004,2005,2006,2007,2008 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include "gsmalg.h"

unsigned char gsvalfunc(int reg) {
    if(reg < 26) return(reg + 'A');
    if(reg < 52) return(reg + 'G');
    if(reg < 62) return(reg - 4);
    if(reg == 62) return('+');
    if(reg == 63) return('/');
    return(0);
}
//No point in having readability here when the function is extremely long. - Ignacio
unsigned char *gsseckey(unsigned char *dst, unsigned char *src, unsigned char *key, int enctype) {
	//I'll never know why this function fails to compile most of the time, you just have to move the "int i, size, keysz;" one line up or down by pressing enter and then just click build. - Sighs -...
	//^ Update: Fixed - http://stackoverflow.com/questions/3695174/visual-studio-2010s-strange-warning-lnk4042
	//Removed the gsmalg.h file from the project and readded it again.
	int i,size,keysz;
	unsigned char enctmp[256], tmp[66], x, y, z, a, b, *p;
    if(!dst) { dst = (unsigned char *)malloc(89); if(!dst) return(NULL); }
    size = strlen((const char *)src);
    if((size < 1) || (size > 65)) { dst[0] = 0; return(dst); }
    keysz = strlen((const char *)key);
    for(i = 0; i < 256; i++) { enctmp[i] = i; }
    a = 0;
    for(i = 0; i < 256; i++) { a += enctmp[i] + key[i % keysz]; x = enctmp[a]; enctmp[a] = enctmp[i]; enctmp[i] = x; }
    a = 0;
    b = 0;
    for(i = 0; src[i]; i++) { a += src[i] + 1; x = enctmp[a]; b += x; y = enctmp[b]; enctmp[b] = x; enctmp[a] = y; tmp[i] = src[i] ^ enctmp[(x + y) & 0xff]; }
    for(size = i; size % 3; size++) { tmp[size] = 0; }
	switch(enctype) { case 1: { for(i = 0; i < size; i++) { tmp[i] = enctype1_data[tmp[i]]; } } case 2: { for(i = 0; i < size; i++) { tmp[i] ^= key[i % keysz]; } } }
    p = dst;
    for(i = 0; i < size; i += 3) { x = tmp[i]; y = tmp[i + 1]; z = tmp[i + 2]; *p++ = gsvalfunc(x >> 2); *p++ = gsvalfunc(((x & 3) << 4) | (y >> 4)); *p++ = gsvalfunc(((y & 15) << 2) | (z >> 6)); *p++ = gsvalfunc(z & 63); }
    *p = 0;
    return(dst);
}

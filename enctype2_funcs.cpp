/*

GS enctype2 servers list decoder/encoder 0.1.2
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    aluigi.org


INTRODUCTION
============
This is the algorithm used to decrypt the data sent by the Gamespy
master server (or any other compatible server) using the enctype 2
method.


USAGE
=====
Add the following prototype at the beginning of your code:

  unsigned char *enctype2_decoder(unsigned char *, unsigned char *, int *);

then use:

        pointer = enctype2_decoder(
            gamekey,        // the gamekey
            buffer,         // all the data received from the master server
            &buffer_len);   // the size of the master server

The return value is a pointer to the decrypted zone of buffer and
buffer_len is modified with the size of the decrypted data.

A simpler way to use the function is just using:

  len = enctype2_wrapper(key, data, data_len);


THANX TO
========
REC (http://www.backerstreet.com/rec/rec.htm) which has helped me in many
parts of the code.


LICENSE
=======
    Copyright 2004-2014 Luigi Auriemma

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>
#include "enctype2_funcs.h"

unsigned char *enctype2_decoder(unsigned char *key, unsigned char *data, int *size) {
    unsigned int    dest[326];
    int             i;
    unsigned char   *datap;

    *data ^= 0xec;
    datap = data + 1;

    for(i = 0; key[i]; i++) datap[i] ^= key[i];

    for(i = 0; i < 326; i++) dest[i] = 0;
    if(*data) encshare4(datap, *data, dest);

    datap += *data;
    *size -= (*data + 1);
    if(*size < 6) {
        *size = 0;
        return(data);
    }

    encshare1(dest, datap, *size);

    *size -= 6;
    return(datap);
}



int enctype2_wrapper(unsigned char *key, unsigned char *data, int size) {
    unsigned char   *p;
    int     i;

    p = enctype2_decoder(key, data, &size);

    //memmove(data, p, size);   // memmove works but is untrusted
    if(p > data) {
        for(i = 0; i < size; i++) {
            data[i] = p[i];
        }
    }
    return(size);
}



/*
data must be big enough to contain also the following bytes: 1 + 8 + 6
*/
int enctype2_encoder(unsigned char *key, unsigned char *data, int size) {
    unsigned int    dest[326];
    int             i;
    unsigned char   *datap;
    int             header_size = 8;

    for(i = size - 1; i >= 0; i--) {
        data[1 + header_size + i] = data[i];
    }
    *data = header_size;
    
    datap = data + 1;
    memset(datap, 0, *data);

    for(i = 256; i < 326; i++) dest[i] = 0;
    encshare4(datap, *data, dest);

    memset(data + 1 + *data + size, 0, 6);
    encshare1(dest, datap + *data, size + 6);

    for(i = 0; key[i]; i++) datap[i] ^= key[i];
    size += 1 + *data + 6;
    *data ^= 0xec;
    return size;
}

/*
    Copyright 2005,2006,2007,2008 Luigi Auriemma

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

void encshare2(unsigned int *tbuff, unsigned int *tbuffp, int len) {
    unsigned int    t1,
                    t2,
                    t3,
                    t4,
                    t5,
                    *limit,
                    *p;

    t2 = tbuff[304];
    t1 = tbuff[305];
    t3 = tbuff[306];
    t5 = tbuff[307];
    limit = tbuffp + len;
    while(tbuffp < limit) {
        p = tbuff + t2 + 272;
        while(t5 < 65536) {
            t1 += t5;
            p++;
            t3 += t1;
            t1 += t3;
            p[-17] = t1;
            p[-1] = t3;
            t4 = (t3 << 24) | (t3 >> 8);
            p[15] = t5;
            t5 <<= 1;
            t2++;
            t1 ^= tbuff[t1 & 0xff];
            t4 ^= tbuff[t4 & 0xff];
            t3 = (t4 << 24) | (t4 >> 8);
            t4 = (t1 >> 24) | (t1 << 8);
            t4 ^= tbuff[t4 & 0xff];
            t3 ^= tbuff[t3 & 0xff];
            t1 = (t4 >> 24) | (t4 << 8);
        }
        t3 ^= t1;
        *tbuffp++ = t3;
        t2--;
        t1 = tbuff[t2 + 256];
        t5 = tbuff[t2 + 272];
        t1 = ~t1;
        t3 = (t1 << 24) | (t1 >> 8);
        t3 ^= tbuff[t3 & 0xff];
        t5 ^= tbuff[t5 & 0xff];
        t1 = (t3 << 24) | (t3 >> 8);
        t4 = (t5 >> 24) | (t5 << 8);
        t1 ^= tbuff[t1 & 0xff];
        t4 ^= tbuff[t4 & 0xff];
        t3 = (t4 >> 24) | (t4 << 8);
        t5 = (tbuff[t2 + 288] << 1) + 1;
    }
    tbuff[304] = t2;
    tbuff[305] = t1;
    tbuff[306] = t3;
    tbuff[307] = t5;
}



void encshare1(unsigned int *tbuff, unsigned char *datap, int len) {
    unsigned char   *p,
                    *s;

    p = s = (unsigned char *)(tbuff + 309);
    encshare2(tbuff, (unsigned int *)p, 16);

    while(len--) {
        if((p - s) == 63) {
            p = s;
            encshare2(tbuff, (unsigned int *)p, 16);
        }
        *datap ^= *p;
        datap++;
        p++;
    }
}



void encshare3(unsigned int *data, int n1, int n2) {
    unsigned int    t1,
                    t2,
                    t3,
                    t4;
    int             i;

    t2 = n1;
    t1 = 0;
    t4 = 1;
    data[304] = 0;
    for(i = 32768; i; i >>= 1) {
        t2 += t4;
        t1 += t2;
        t2 += t1;
        if(n2 & i) {
            t2 = ~t2;
            t4 = (t4 << 1) + 1;
            t3 = (t2 << 24) | (t2 >> 8);
            t3 ^= data[t3 & 0xff];
            t1 ^= data[t1 & 0xff];
            t2 = (t3 << 24) | (t3 >> 8);
            t3 = (t1 >> 24) | (t1 << 8);
            t2 ^= data[t2 & 0xff];
            t3 ^= data[t3 & 0xff];
            t1 = (t3 >> 24) | (t3 << 8);
        } else {
            data[data[304] + 256] = t2;
            data[data[304] + 272] = t1;
            data[data[304] + 288] = t4;
            data[304]++;
            t3 = (t1 << 24) | (t1 >> 8);
            t2 ^= data[t2 & 0xff];
            t3 ^= data[t3 & 0xff];
            t1 = (t3 << 24) | (t3 >> 8);
            t3 = (t2 >> 24) | (t2 << 8);
            t3 ^= data[t3 & 0xff];
            t1 ^= data[t1 & 0xff];
            t2 = (t3 >> 24) | (t3 << 8);
            t4 <<= 1;
        }
    }
    data[305] = t2;
    data[306] = t1;
    data[307] = t4;
    data[308] = n1;
    //    t1 ^= t2;
}



void encshare4(unsigned char *src, int size, unsigned int *dest) {
    unsigned int    tmp;
    int             i;
    unsigned char   pos,
                    x,
                    y;

    for(i = 0; i < 256; i++) dest[i] = 0;

    for(y = 0; y < 4; y++) {
        for(i = 0; i < 256; i++) {
            dest[i] = (dest[i] << 8) + i;
        }

        for(pos = y, x = 0; x < 2; x++) {
            for(i = 0; i < 256; i++) {
                tmp = dest[i];
                pos += tmp + src[i % size];
                dest[i] = dest[pos];
                dest[pos] = tmp;
            }
        }
    }

    for(i = 0; i < 256; i++) dest[i] ^= i;

    encshare3(dest, 0, 0);
}

/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ENDIAN_RW_H__
#define __ENDIAN_RW_H__

#define GETL64(buf) \
   (((unsigned long long)(buf)[0]      ) | \
    ((unsigned long long)(buf)[1] <<  8) | \
    ((unsigned long long)(buf)[2] << 16) | \
    ((unsigned long long)(buf)[3] << 24) | \
    ((unsigned long long)(buf)[4] << 32) | \
    ((unsigned long long)(buf)[5] << 40) | \
    ((unsigned long long)(buf)[6] << 48) | \
    ((unsigned long long)(buf)[7] << 56))

#define GETL32(buf) \
   (((unsigned int)(buf)[0]      ) | \
    ((unsigned int)(buf)[1] <<  8) | \
    ((unsigned int)(buf)[2] << 16) | \
    ((unsigned int)(buf)[3] << 24))

#define GETL24(buf) \
   (((unsigned int)(buf)[0]      ) | \
    ((unsigned int)(buf)[1] <<  8) | \
    ((unsigned int)(buf)[2] << 16))

#define GETL16(buf) \
   (((unsigned short)(buf)[0]     ) | \
    ((unsigned short)(buf)[1] << 8))

#define PUTL16(buf, s) \
   (buf)[0] = ((s)     ) & 0xff; \
   (buf)[1] = ((s) >> 8) & 0xff;

#define PUTL32(buf, l) \
   (buf)[0] = ((l)      ) & 0xff; \
	(buf)[1] = ((l) >>  8) & 0xff; \
	(buf)[2] = ((l) >> 16) & 0xff; \
	(buf)[3] = ((l) >> 24) & 0xff;

#define PUTL64(buf, ll) \
   (buf)[0] = ((ll)      ) & 0xff; \
	(buf)[1] = ((ll) >>  8) & 0xff; \
	(buf)[2] = ((ll) >> 16) & 0xff; \
	(buf)[3] = ((ll) >> 24) & 0xff; \
   (buf)[4] = ((ll) >> 32) & 0xff; \
   (buf)[5] = ((ll) >> 40) & 0xff; \
   (buf)[6] = ((ll) >> 48) & 0xff; \
   (buf)[7] = ((ll) >> 56) & 0xff;

#endif /* __ENDIAN_RW_H__ */

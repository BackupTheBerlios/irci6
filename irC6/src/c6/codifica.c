/***************************************************************************
                          protosupport.cpp  -  description
                             -------------------
    begin                : Fri Mar 7 2003
    copyright            : (C) 2003 by Giorgio A.
    email                : openc6@hotmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "defs.h"
#include "str_missing.h"
#include "blowfish.h"
#include "md5.h"

#define BYTE unsigned char

static void GenerKey(unsigned char *pDest,unsigned char *pSorg){
    struct md5_context ctx;

    md5_starts( &ctx );
    md5_update( &ctx, (uint8 *) pSorg, strlen((char *)pSorg));
    md5_finish( &ctx, pDest );
}

/* FUNZIONE DA USARE PER OTTENERE LA CODIFICA */
static void Codifica(unsigned char *StrCode,unsigned char *StrKeyServer,unsigned char *StrDest,unsigned int Psw)
{
  unsigned char Strtemp[8]="ANOCI";
  unsigned char tt[30];
  int i;

   if (Psw)
     {
       int y = strlen((char*)StrCode);
       memcpy(tt,StrCode,y);
       tt[y] = StrKeyServer[0];
       tt[y+1] = StrKeyServer[2];
       tt[y+2] =0;
     }
     else
     {
       memcpy(tt,Strtemp,5);
       tt[5] = tolower(StrCode[strlen((char *)StrCode)-1]);
       tt[6] = tolower(StrCode[0]);
       tt[7] = StrKeyServer[4];
       tt[8] = StrKeyServer[2];
       tt[9] = 0;
     }

     GenerKey(StrDest,tt);

  	 for(i=0;i<16;i++)
       StrDest[i] = (StrDest[i] % 0x5E)+0x20;
}

static void XorCode(BYTE *pSorg,BYTE *pDest,BYTE *pKeyOrd,int len)
{
  int i;
	for( i=0; i<len; i++)
    pDest[i]=pSorg[i]^pKeyOrd[i % 7];
}

static void OrderKey(BYTE *dest,BYTE *source)
{
  int step = source[5] % 7;
  int i=1;

  dest[0]=source[0];
  while (i < 8)
    {
      dest[i] = source[(i*step) % 7];
      i++;
    }
}

#define WORD_WRITE(ptr,value) do { *ptr = (BYTE)((value & 0xff00) >> 8); ptr++; *ptr = (BYTE)(value & 0x00ff) ; ptr++; } while (0)


static void BlowFishEncode(int len,BYTE *source,int len_key,BYTE* init)
{
  BLOWFISH_CTX ctx;
  unsigned long lvalue,rvalue;
  int i;

  Blowfish_Init(&ctx,init,len_key);

  for (i=0; i < (len /8); i++)
    {
      lvalue = *source << 24 | *(source+1) << 16 | +*(source+2) << 8 | *(source+3);
      rvalue = *(source+4) << 24 | *(source+5) << 16 | +*(source+6) << 8 | *(source+7);

      Blowfish_Encrypt(&ctx,&lvalue,&rvalue);
      WORD_WRITE(source,lvalue >> 16);
      WORD_WRITE(source,lvalue & 0x0000ffff);
      WORD_WRITE(source,rvalue >> 16);
      WORD_WRITE(source,rvalue & 0x0000ffff);
    }
}

static void BlowFishDecode(int len,BYTE *source,int len_key,BYTE* init)
{
  BLOWFISH_CTX ctx;
  unsigned long lvalue,rvalue;
  int i;

  Blowfish_Init(&ctx,init,len_key);

  for (i=0; i < (len /8); i++)
    {
      lvalue = *source << 24 | *(source+1) << 16 | +*(source+2) << 8 | *(source+3);
      rvalue = *(source+4) << 24 | *(source+5) << 16 | +*(source+6) << 8 | *(source+7);

      Blowfish_Decrypt(&ctx,&lvalue,&rvalue);
      WORD_WRITE(source,lvalue >> 16);
      WORD_WRITE(source,lvalue & 0x0000ffff);
      WORD_WRITE(source,rvalue >> 16);
      WORD_WRITE(source,rvalue & 0x0000ffff);
    }
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

void c6_nick_encode(unsigned char *src, unsigned char *dst, unsigned char *server_key)
{
	Codifica(src, server_key, dst, 0);
}

void c6_pass_encode(unsigned char *src, unsigned char *dst, unsigned char *server_key)
{
	Codifica(src, server_key, dst, 1);
}

void c6_reorder_key_server(const unsigned char *unordered, unsigned char *ordered)
{
	OrderKey(ordered, (unsigned char *)unordered);
}

void c6_set_md5_key(unsigned char *md5_key, unsigned char *key)
{
	int i;

	GenerKey(md5_key, key);

	for(i = 0; i < 16; i++)
		md5_key[i] = (md5_key[i] % 0x5E) + 0x20;
}

void c6_encode_xml_blob(unsigned char *dst, size_t dst_len, unsigned char *md5_key)
{
	BlowFishEncode(dst_len, dst, 16, md5_key);
}

void c6_decode_xml_blob(unsigned char *dst, size_t dst_len, unsigned char *md5_key)
{
	BlowFishDecode(dst_len, dst, 16, md5_key);
}

void c6_packet_encode(unsigned char *dst, unsigned char *src, int len, unsigned char *reordered_key)
{
	XorCode(src, dst, reordered_key, len);
}

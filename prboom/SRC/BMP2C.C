#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#ifndef __GNUC__
#define __attribute__(x)
#define __inline__
#endif

#define XPARENT (0xf7)	/* palette index representing transparent in BMP */

typedef struct stPicData
{
	short pwid;
	short phgt;
	short pxoff;
	short pyoff;
	
	long *coloffs;
	unsigned char *posts;
	long postlen;
} PicData;

#define BI_RGB      0L
#define BI_RLE8     1L
#define BI_RLE4     2L

typedef unsigned short UINT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned char BYTE;
typedef unsigned char UBYTE;

typedef struct tagBITMAPFILEHEADER
{
    UINT    bfType;				  	 	//	2
    DWORD   bfSize;				   	 	//	4
    UINT    bfReserved1;		   	//	2
    UINT    bfReserved2;		   	//	2
    DWORD   bfOffBits;			   	//	4
} __attribute__ ((packed)) BITMAPFILEHEADER;				   //	14

typedef struct tagBITMAPINFOHEADER
{
    DWORD   biSize;				   		//	4
    LONG    biWidth;			   		//	4
    LONG    biHeight;			   		//	4
    WORD    biPlanes;			   		//	2
    WORD    biBitCount;			   	//	2
    DWORD   biCompression;		  //	4
    DWORD   biSizeImage;		   	//	4
    LONG    biXPelsPerMeter;	  //	4
    LONG    biYPelsPerMeter;	  //	4
		DWORD   biClrUsed;			   	//	4
    DWORD   biClrImportant;		  //	4
} __attribute__ ((packed)) BITMAPINFOHEADER;				   //	40

typedef struct tagRGBQUAD
{
    UBYTE    rgbBlue;
    UBYTE    rgbGreen;
    UBYTE    rgbRed;
    UBYTE    rgbReserved;
} __attribute__ ((packed)) RGBQUAD;

// Convert a rectangular array of numbers to a DOOM format picture
// bytes 0-255 represent palette colors as usual, XPARENT is transparency.
//
// memory is allocated on p->coloffs and p->posts
// max: 256k+rwid actual: postlen+4*(1+rwid)

void ConvertToPicture(BYTE *bytes,int logw,int rwid,int logh,PicData *p,int pf)
{
	int i,j,n;
	int pixcnt;
	int postlen;

	p->pwid = rwid;
	p->phgt = logh;
	p->pxoff = pf? rwid/2 - 1 : 0;
	p->pyoff = pf? logh - 5 : 0;

	p->coloffs = (long *)malloc(logw*sizeof(int));
	p->posts = (unsigned char *)malloc(1024*256*sizeof(unsigned char));

	n=postlen=0;
	for (j=0;j<rwid;j++)							// do each column
	{
		p->coloffs[j] = 4*sizeof(short) + p->pwid*sizeof(int) + postlen;
		pixcnt = 0;
		for (i=0;i<logh;i++)
		{
			if (bytes[logw*i+j]==XPARENT)
			{
				if (pixcnt>0)
				{
					p->posts[postlen++]=0;		// add fill byte at end
					pixcnt=0;
				}
			}
			else
			{
				if (pixcnt==0) 							// start a post
				{
					p->posts[postlen++] = i;
					n = postlen++;
					p->posts[n] = 1;
					p->posts[postlen++] = 0;  // add fill byte at start
				}
				else p->posts[n]++;

				// add pixel to current post

				p->posts[postlen++] = bytes[logw*i+j];
				pixcnt++;
			}
		}
		if (pixcnt>0)										// terminate last post
			p->posts[postlen++]=0;
		p->posts[postlen++]=255;				// terminate column
	}
	p->posts = (unsigned char *)realloc(p->posts,postlen);
	p->postlen = postlen;
}

// Reads BMP file
// Header is read for real width, and logical height
// Logical width is next multiple of 4 greater than or equal to real width
// Bitmap is written to short array *out, allocated to fit
// *out is allocated and read as logical width by logical height
//
// memory is allocated on *out
// max: logw*logh*sizeof(short)

void ReadBMP(BYTE **out,int *logw,int *realw,int *logh,char *bmpname)
{
	int i,n;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	FILE *st;
	char *buffer=NULL;
	char *p=NULL;
	RGBQUAD pal[256];

	*logw = *logh = *realw = 0;
  st = fopen(bmpname,"rb");
	if (st!=NULL)
	{
		fread(&bmfh.bfType,sizeof(bmfh.bfType),1,st);
		fread(&bmfh.bfSize,sizeof(bmfh.bfSize),1,st);
		fread(&bmfh.bfReserved1,sizeof(bmfh.bfReserved1),1,st);
		fread(&bmfh.bfReserved2,sizeof(bmfh.bfReserved2),1,st);
		fread(&bmfh.bfOffBits,sizeof(bmfh.bfOffBits),1,st);
		fread(&bmih.biSize,sizeof(bmih.biSize),1,st);
		fread(&bmih.biWidth,sizeof(bmih.biWidth),1,st);
		fread(&bmih.biHeight,sizeof(bmih.biHeight),1,st);
		fread(&bmih.biPlanes,sizeof(bmih.biPlanes),1,st);
		fread(&bmih.biBitCount,sizeof(bmih.biBitCount),1,st);
		fread(&bmih.biCompression,sizeof(bmih.biCompression),1,st);
		fread(&bmih.biSizeImage,sizeof(bmih.biSizeImage),1,st);
		fread(&bmih.biXPelsPerMeter,sizeof(bmih.biXPelsPerMeter),1,st);
		fread(&bmih.biYPelsPerMeter,sizeof(bmih.biYPelsPerMeter),1,st);
		fread(&bmih.biClrUsed,sizeof(bmih.biClrUsed),1,st);
		fread(&bmih.biClrImportant,sizeof(bmih.biClrImportant),1,st);
		fread(pal,sizeof(RGBQUAD),256,st);

		*realw = bmih.biWidth;
		*logw = 4*((bmih.biWidth+3)/4);
		*logh = bmih.biHeight;

		n = (*logw)*(*logh);

		buffer = malloc(n);
		fseek(st,bmfh.bfOffBits,SEEK_SET);
		fread(buffer,n,1,st);

		*out = (BYTE *)malloc(n);

		for (i=(*logh)-1;i>=0;i--)
			memmove((*out)+((*logh)-1-i)*(*logw),buffer+(*logw)*i,*logw);
		fclose(st);
		free(buffer);
	}
	else fprintf(stderr,"ERROR: bitmap file %s not found\n",bmpname);
}

// Read a .BMP file and embed in a wad as a DOOM picture
// memory balanced except directory extension

void WritePic(char *path,FILE *st,int pf)
{
	int n,t;
	BYTE *out = NULL;
	int logw=0,logh=0,rwid=0;
	PicData pic;
	char name[256],*p;
	BYTE *q;

	if (path && *path)
	{
		ReadBMP(&out,&logw,&rwid,&logh,path);	// allocs out
		if (logw>0)
		{
			ConvertToPicture(out,logw,rwid,logh,&pic,pf);

			strcpy(name,path[1]==':'? path+2 : path);	// strip drive, if any
			p = strrchr(name,'\\');										// strip dirs, if any
			if (p) strcpy(name,p+1);
			p = strrchr(name,'.');										// strip ext, if any
			if (p) *p='\0';

			// code to write pic as C data here

			fprintf(st,"static const char %s[]=\n{\n\t",name);

			q = (BYTE *)&pic.pwid;
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,",*q++);
			fprintf(st,"%3d,\n\t",*q++);

			q = (BYTE *)pic.coloffs;
			t = pic.pwid*sizeof(long);
			for (n=0;n<t;n++)
			{
				fprintf(st,"%3d,",*q++);
			  if ((n&15)==15)
					fprintf(st,"\n\t");
			}
	
			q = pic.posts;
			for (;n<pic.postlen+t;n++)
			{
				fprintf(st,"%3d,",*q++);
				if (n==t+pic.postlen-1)
					fprintf(st,"\n};\n\n");
			  else if ((n&15)==15)
					fprintf(st,"\n\t");
			}
			free(pic.coloffs);
			free(pic.posts);
		}
		else
		{
			fprintf(stderr,"ERROR: Can't open file: %s\n",path);
			return;
		}
		free(out);
	}
}


// write all .BMP files specified as input (possibly wildcarded)
// as a collection of C arrays

int main(int argc,char **argv)
{
	int i;
	char name[256];
	FILE *st;
	int pf;

	if (argc<2)
	{
		printf("Usage: BMP2C [/p] name1.bmp...\n");
		printf("name may contain wildcards\n");
		printf("/p will insert patch style offsets, else 0\n");
		exit(1);
	}
	st = fopen("cdata.c","w");
	if (st)
	{
		pf = 0;
		if (argv[1][0] == '-' || argv[1][0]=='/' && tolower(argv[1][1])=='p')
			pf = 1;

		for (i=1+pf;i<argc;i++)
		{
			strcpy(name,argv[i]);
			WritePic(name,st,pf);
		}
	}
	else
		fprintf(stderr,"Cannot open CDATA.C for output\n");
	return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// trim spaces off left of string

void LeftTrim(char *str)
{
	int k,n;

	k = strlen(str);
	if (k)
	{
		n = strspn(str," \t");
		memmove(str,str+n, k-n+1);
	}
}

// trim spaces off right of string

void RightTrim(char *str)
{
	int k,n;

	n = k = strlen(str);
	if (k)
	{
		n--;
		while (n>=0 && (str[n]==' ' || str[n]=='\t')) n--;
		str[n+1]='\0';
	}
}

// trim spaces off both ends of a string

void Trim(char *str)
{
	RightTrim(str);
	LeftTrim(str);
}

// If the extension is missing, add the extension ext
// If the extension is present, leave it be

void SupplyExt(char *str,char *ext)
{
	char *p,*q;

	if (!str || !*str)
		return;

	p = strrchr(str,'\\');		// last backslash
	q = strrchr(str,'.');		// last period
	if (p && q && q<p)			// if is a backslash, and the . is before it
		strcat(str,ext);		// add an extension, since there is none
	else if (!q)
		strcat(str,ext);		// if no period, add the extension
}

int main(int argc,char **argv)
{
	char inpname[256],*p,*q,*tok;
	char secname[256]="";
	char string1[9],string2[9];
	char buffer[256];
	int lineno=0,inum;
	short num;
	char true=1,false=0;
	char zero[9]={0,0,0,0,0,0,0,0,0};
	FILE *st=NULL,*outst=NULL;

	if (argc<2)
	{
		printf("\nUsage: SWANTBL swanfile[.DAT]\n");
		printf("Creates SWITCHES.LMP and ANIMATED.LMP from text input file\n");
		exit(1);
	}

	strcpy(inpname,argv[1]);
	SupplyExt(inpname,".DAT");

	st = fopen(inpname,"r");
	if (st)
	{
		while ((p=fgets(buffer,256,st)))
		{
			lineno++;
			Trim(p);
			if (p[strlen(p)-1]=='\n') p[strlen(p)-1]='\0';
			if (*p=='[')
			{
				q = strchr(p,']');
				if (!q)
				{
					printf("Unclosed section delimiter in line %d, missing ]\n",lineno);
					fclose(st);
					exit(1);
				}

				*q = '\0';
				strcpy(secname,p+1);
				if (!stricmp(secname,"SWITCHES"))
				{
					outst=fopen("SWITCHES.LMP","wb");
				}
				else if (!stricmp(secname,"FLATS"))
				{
					;
				}
				else if (!stricmp(secname,"TEXTURES"))
				{
					;
				}
				else 
				{
					printf("Bad or duplicate section header in line %d, use [SWITCHES], [FLATS], or [TEXTURES] once\n",lineno);
					fclose(st);
					exit(1);
				}
			}
			else if (*p=='\n' || *p=='\r' || *p=='#' || *p==';' || *p==0)
			{
				continue;
			}
			else if (!isdigit(*p))
			{
				printf("Bad syntax, line %d does not begin with a number\n",lineno);
				exit(1);
			}
			else if (!stricmp("SWITCHES",secname))
			{
				tok = strtok(p," \t,");
				if (!tok)
				{
					printf("Bad syntax on line %d, missing first token\n",lineno);
			    fclose(st);
					fclose(outst);
				  exit(1);
				}
				else
				{
					num = inum = atoi(tok);
					tok = strtok(NULL," \t,");
					if (!tok)
					{
						printf("Bad syntax on line %d, missing second token\n",lineno);
			    	fclose(st);
						fclose(outst);
				  	exit(1);
					}
					else
					{
						strncpy(string1,tok,9);
						tok = strtok(NULL," \t,");
						if (!tok)
						{
							printf("Bad syntax on line %d, missing third token\n",lineno);
			    		fclose(st);
							fclose(outst);
				  		exit(1);
						}
						else strncpy(string2,tok,9);
					}
				}
				if (!stricmp(secname,"SWITCHES"))
				{
					fwrite(string1,9,1,outst);
					fwrite(string2,9,1,outst);
					fwrite(&num,2,1,outst);
				}
				else
				{
					printf("Error: data at line %d found outside section\n",lineno);
					fclose(st);
					fclose(outst);
				}
			}
		}
		if (stricmp("SWITCHES",secname))
		{
			fwrite(zero,9,1,outst);
			fwrite(zero,9,1,outst);
			fwrite(zero,2,1,outst);
		}
		if (outst) fclose(outst);
		outst=NULL;

		fclose(st);
		st = fopen(inpname,"r");
		while (st && (p=fgets(buffer,256,st)))
		{
			lineno++;
			Trim(p);
			if (p[strlen(p)-1]=='\n') p[strlen(p)-1]='\0';
			if (*p=='[')
			{
				q = strchr(p,']');
				if (!q)
				{
					printf("Unclosed section delimiter in line %d, missing ]\n",lineno);
					fclose(st);
					exit(1);
				}

				*q = '\0';
				strcpy(secname,p+1);
				if (!stricmp(secname,"SWITCHES"))
				{
					;
				}
				else if (!stricmp(secname,"FLATS"))
				{
					if (!outst)
						outst=fopen("ANIMATED.LMP","wb");
				}
				else if (!stricmp(secname,"TEXTURES"))
				{
					if (!outst)
						outst=fopen("ANIMATED.LMP","wb");
				}
				else 
				{
					printf("Bad or section header in line %d, use [SWITCHES], [FLATS], or [TEXTURES] once\n",lineno);
					fclose(st);
					exit(1);
				}
			}
			else if (*p=='\n' || *p=='\r' || *p=='#' || *p==';' || *p==0)
			{
				continue;
			}
			else if (!isdigit(*p))
			{
				printf("Bad syntax, line %d does not begin with a number\n",lineno);
				exit(1);
			}
			else if (!stricmp("FLATS",secname) || !stricmp("TEXTURES",secname))
			{
				tok = strtok(p," \t,");
				if (!tok)
				{
					printf("Bad syntax on line %d, missing first token\n",lineno);
			    fclose(st);
					fclose(outst);
				  exit(1);
				}
				else
				{
					num = inum = atoi(tok);
					tok = strtok(NULL," \t,");
					if (!tok)
					{
						printf("Bad syntax on line %d, missing second token\n",lineno);
			    	fclose(st);
						fclose(outst);
				  	exit(1);
					}
					else
					{
						strncpy(string1,tok,9);
						tok = strtok(NULL," \t,");
						if (!tok)
						{
							printf("Bad syntax on line %d, missing third token\n",lineno);
			    		fclose(st);
							fclose(outst);
				  		exit(1);
						}
						else strncpy(string2,tok,9);
					}
				}
				if (!stricmp(secname,"FLATS"))
				{
					fwrite(&false,1,1,outst);
					fwrite(string1,9,1,outst);
					fwrite(string2,9,1,outst);
					fwrite(&inum,4,1,outst);
				}
				else if (!stricmp(secname,"TEXTURES"))
				{
					fwrite(&true,1,1,outst);
					fwrite(string1,9,1,outst);
					fwrite(string2,9,1,outst);
					fwrite(&inum,4,1,outst);
				}
				else
				{
					printf("Error: data at line %d found outside section\n",lineno);
					fclose(st);
					fclose(outst);
					exit(1);
				}
			}
		}
		if (!stricmp(secname,"FLATS"))
		{
			fwrite("\xff\xff\xff\xff",4,1,outst);
		}
		else if (!stricmp(secname,"TEXTURES"))
		{
			fwrite("\xff\xff\xff\xff",4,1,outst);
		}
	}
	else
	{
		printf("Cannot open %s for reading\n",inpname);
		exit(1);
	}

	if (st) fclose(st);
	if (outst)
		fclose(outst);
	return 0;
}

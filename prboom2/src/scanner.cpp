// Copyright (c) 2010, Braden "Blzut3" Obrzut <admin@maniacsvault.net>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the <organization> nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "doomtype.h"
#include "scanner.h"

const char* const Scanner::TokenNames[TK_NumSpecialTokens] =
{
	"Identifier",
	"String Constant",
	"Integer Constant",
	"Float Constant",
	"Boolean Constant",
	"Logical And",
	"Logical Or",
	"Equals",
	"Not Equals",
	"Greater Than or Equals"
	"Less Than or Equals",
	"Left Shift",
	"Right Shift"
};

static void __ScannerStandardError(const char* message, ...)
{
	va_list list;
	va_start(list, message);
	vfprintf(stderr, message, list);
	va_end(list);
}
void (*Scanner::error)(const char*, ...) = __ScannerStandardError;

Scanner::Scanner(const char* data, int length) : line(1), lineStart(0), logicalPosition(0), tokenLine(1), tokenLinePosition(0), scanPos(0), needNext(true)
{
	if(length == -1)
		length = strlen(data);
	this->length = length;
	this->data = new char[length];
	memcpy(this->data, data, length);
	string = NULL;

	CheckForWhitespace();
}

Scanner::~Scanner()
{
	if (string != NULL) delete[] string;
	delete[] data;
}

void Scanner::SetString(char **ptr, const char *start, unsigned int length)
{
	if (length == -1)
		length = strlen(start);
	if (*ptr != NULL) free(*ptr);
	*ptr = (char*)malloc(length + 1);
	memcpy(*ptr, start, length);
	(*ptr)[length] = 0;
}

void Scanner::CheckForWhitespace()
{
	int comment = 0; // 1 = till next new line, 2 = till end block
	while(scanPos < length)
	{
		char cur = data[scanPos];
		char next = scanPos+1 < length ? data[scanPos+1] : 0;
		if(comment == 2)
		{
			if(cur != '*' || next != '/')
			{
				if(cur == '\n' || cur == '\r')
				{
					scanPos++;
					if(comment == 1)
						comment = 0;

					// Do a quick check for Windows style new line
					if(cur == '\r' && next == '\n')
						scanPos++;
					IncrementLine();
				}
				else
					scanPos++;
			}
			else
			{
				comment = 0;
				scanPos += 2;
			}
			continue;
		}

		if(cur == ' ' || cur == '\t' || cur == 0)
			scanPos++;
		else if(cur == '\n' || cur == '\r')
		{
			scanPos++;
			if(comment == 1)
				comment = 0;

			// Do a quick check for Windows style new line
			if(cur == '\r' && next == '\n')
				scanPos++;
			IncrementLine();
			//CheckForMeta();
		}
		else if(cur == '/' && comment == 0)
		{
			switch(next)
			{
				case '/':
					comment = 1;
					break;
				case '*':
					comment = 2;
					break;
				default:
					return;
			}
			scanPos += 2;
		}
		else
		{
			if(comment == 0)
				return;
			else
				scanPos++;
		}
	}
}

bool Scanner::CheckToken(char token)
{
	if(needNext)
	{
		if(!GetNextToken(false))
			return false;
	}

	// An int can also be a float.
	if(nextState.token == token || (nextState.token == TK_IntConst && token == TK_FloatConst))
	{
		needNext = true;
		ExpandState();
		return true;
	}
	needNext = false;
	return false;
}

void Scanner::ExpandState()
{
	logicalPosition = scanPos;
	CheckForWhitespace();

	SetString(&string, nextState.string, -1);
	number = nextState.number;
	decimal = nextState.decimal;
	boolean = nextState.boolean;
	token = nextState.token;
	tokenLine = nextState.tokenLine;
	tokenLinePosition = nextState.tokenLinePosition;
}

void Scanner::SaveState(Scanner &savedstate)
{
	// This saves the entire parser state except for the data pointer.
	if (savedstate.string != NULL) free(savedstate.string);
	if (savedstate.nextState.string != NULL) free(savedstate.nextState.string);
	memcpy(&savedstate, this, sizeof(*this));
	savedstate.string = strdup(string);
	savedstate.nextState.string = strdup(nextState.string);
	savedstate.data = NULL;
}

void Scanner::RestoreState(Scanner &savedstate)
{
	if (savedstate.data == NULL)
	{
		char *saveddata = data;
		savedstate.SaveState(*this);
		data = saveddata;
	}
}

bool Scanner::GetNextToken(bool expandState)
{
	if(!needNext)
	{
		needNext = true;
		if(expandState)
			ExpandState();
		return true;
	}

	nextState.tokenLine = line;
	nextState.tokenLinePosition = scanPos - lineStart;
	nextState.token = TK_NoToken;
	if(scanPos >= length)
	{
		if(expandState)
			ExpandState();
		return false;
	}

	int start = scanPos;
	int end = scanPos;
	int integerBase = 10;
	bool floatHasDecimal = false;
	bool floatHasExponent = false;
	bool stringFinished = false; // Strings are the only things that can have 0 length tokens.

	char cur = data[scanPos++];
	// Determine by first character
	if(cur == '_' || (cur >= 'A' && cur <= 'Z') || (cur >= 'a' && cur <= 'z'))
		nextState.token = TK_Identifier;
	else if(cur >= '0' && cur <= '9')
	{
		if(cur == '0')
			integerBase = 8;
		nextState.token = TK_IntConst;
	}
	else if(cur == '.')
	{
		floatHasDecimal = true;
		nextState.token = TK_FloatConst;
	}
	else if(cur == '"')
	{
		end = ++start; // Move the start up one character so we don't have to trim it later.
		nextState.token = TK_StringConst;
	}
	else
	{
		end = scanPos;
		nextState.token = cur;

		// Now check for operator tokens
		if(scanPos < length)
		{
			char next = data[scanPos];
			if(cur == '&' && next == '&')
				nextState.token = TK_AndAnd;
			else if(cur == '|' && next == '|')
				nextState.token = TK_OrOr;
			else if(cur == '<' && next == '<')
				nextState.token = TK_ShiftLeft;
			else if(cur == '>' && next == '>')
				nextState.token = TK_ShiftRight;
			//else if(cur == '#' && next == '#')
			//	nextState.token = TK_MacroConcat;
			else if(next == '=')
			{
				switch(cur)
				{
					case '=':
						nextState.token = TK_EqEq;
						break;
					case '!':
						nextState.token = TK_NotEq;
						break;
					case '>':
						nextState.token = TK_GtrEq;
						break;
					case '<':
						nextState.token = TK_LessEq;
						break;
					default:
						break;
				}
			}

			if(nextState.token != cur)
			{
				scanPos++;
				end = scanPos;
			}
		}
	}

	if(start == end)
	{
		while(scanPos < length)
		{
			cur = data[scanPos];
			switch(nextState.token)
			{
				default:
					break;
				case TK_Identifier:
					if(cur != '_' && (cur < 'A' || cur > 'Z') && (cur < 'a' || cur > 'z') && (cur < '0' || cur > '9'))
						end = scanPos;
					break;
				case TK_IntConst:
					if(cur == '.' || (scanPos-1 != start && cur == 'e'))
						nextState.token = TK_FloatConst;
					else if((cur == 'x' || cur == 'X') && scanPos-1 == start)
					{
						integerBase = 16;
						break;
					}
					else
					{
						switch(integerBase)
						{
							default:
								if(cur < '0' || cur > '9')
									end = scanPos;
								break;
							case 8:
								if(cur < '0' || cur > '7')
									end = scanPos;
								break;
							case 16:
								if((cur < '0' || cur > '9') && (cur < 'A' || cur > 'F') && (cur < 'a' || cur > 'f'))
									end = scanPos;
								break;
						}
						break;
					}
				case TK_FloatConst:
					if(cur < '0' || cur > '9')
					{
						if(!floatHasDecimal && cur == '.')
						{
							floatHasDecimal = true;
							break;
						}
						else if(!floatHasExponent && cur == 'e')
						{
							floatHasDecimal = true;
							floatHasExponent = true;
							if(scanPos+1 < length)
							{
								char next = data[scanPos+1];
								if((next < '0' || next > '9') && next != '+' && next != '-')
									end = scanPos;
								else
									scanPos++;
							}
							break;
						}
						end = scanPos;
					}
					break;
				case TK_StringConst:
					if(cur == '"')
					{
						stringFinished = true;
						end = scanPos;
						scanPos++;
					}
					else if(cur == '\\')
						scanPos++; // Will add two since the loop automatically adds one
					break;
			}
			if(start == end && !stringFinished)
				scanPos++;
			else
				break;
		}
	}

	if(end-start > 0 || stringFinished)
	{
		SetString(&nextState.string, data+start, end-start);
		if(nextState.token == TK_FloatConst)
		{
			nextState.decimal = atof(nextState.string);
			nextState.number = static_cast<int> (nextState.decimal);
			nextState.boolean = (nextState.number != 0);
		}
		else if(nextState.token == TK_IntConst)
		{
			nextState.number = strtol(nextState.string, NULL, integerBase);
			nextState.decimal = nextState.number;
			nextState.boolean = (nextState.number != 0);
		}
		else if(nextState.token == TK_Identifier)
		{
			// Identifiers should be case insensitive.
			char *p = nextState.string;
			while (*p)
			{
				*p = tolower(*p);
				p++;
			}
			// Check for a boolean constant.
			if(strcmp(nextState.string, "true") == 0)
			{
				nextState.token = TK_BoolConst;
				nextState.boolean = true;
			}
			else if (strcmp(nextState.string, "false") == 0)
			{
				nextState.token = TK_BoolConst;
				nextState.boolean = false;
			}
		}
		else if(nextState.token == TK_StringConst)
		{
			Unescape(nextState.string);
		}
		if(expandState)
			ExpandState();
		return true;
	}
	nextState.token = TK_NoToken;
	if(expandState)
		ExpandState();
	return false;
}

void Scanner::IncrementLine()
{
	line++;
	lineStart = scanPos;
}

void Scanner::Error(int token)
{
	if (token < TK_NumSpecialTokens && this->token < TK_NumSpecialTokens)
		error("%d:%d:Expected '%s' but got '%s' instead.", GetLine(), GetLinePos(), TokenNames[token], TokenNames[this->token]);
	else if (token < TK_NumSpecialTokens && this->token >= TK_NumSpecialTokens)
		error("%d:%d:Expected '%s' but got '%c' instead.", GetLine(), GetLinePos(), TokenNames[token], this->token);
	else if (token >= TK_NumSpecialTokens && this->token < TK_NumSpecialTokens)
		error("%d:%d:Expected '%c' but got '%s' instead.", GetLine(), GetLinePos(), token, TokenNames[this->token]);
	else
		error("%d:%d:Expected '%c' but got '%c' instead.", GetLine(), GetLinePos(), token, this->token);
}

void Scanner::Error(const char *mustget)
{
	if (token < TK_NumSpecialTokens && this->token < TK_NumSpecialTokens)
		error("%d:%d:Expected '%s' but got '%s' instead.", GetLine(), GetLinePos(), mustget, TokenNames[this->token]);
	else
		error("%d:%d:Expected '%s' but got '%c' instead.", GetLine(), GetLinePos(), mustget, this->token);
}

void Scanner::ErrorF(const char *msg, ...)
{
	char buffer[1024];
	va_list ap;
	va_start(ap, msg);
	vsnprintf(buffer, 1024, msg, ap);
	va_end(ap);
	error("%d:%d:%s.", GetLine(), GetLinePos(), buffer);
}

void Scanner::MustGetToken(char token)
{
	if(!CheckToken(token))
	{
		ExpandState();
		Error(token);
	}
}

void Scanner::MustGetIdentifier(const char *ident)
{
	if (!CheckToken(TK_Identifier) || strcasecmp(string, ident))
	{
		Error(ident);
		return;
	}
}

// Convenience helpers that parse an entire number including a leading minus or plus sign
bool Scanner::ScanInteger()
{
	bool neg = false;
	if (!GetNextToken()) 
	{
		return false;
	}
	if (token == '-')
	{
		if (!GetNextToken()) 
		{
			return false;
		}
		neg = true;
	}
	else if (token == '+')
	{
		if (!GetNextToken()) 
		{
			return false;
		}
	}
	if (token != TK_IntConst) 
	{
		return false;
	}
	if (neg)
	{
		number = -number;
		decimal = -decimal;
	}
	return true;
}

bool Scanner::ScanFloat()
{
	bool neg = false;
	if (!GetNextToken()) 
	{
		return false;
	}
	if (token == '-')
	{
		if (!GetNextToken()) 
		{
			return false;
		}
		neg = true;
	}
	else if (token == '+')
	{
		if (!GetNextToken()) 
		{
			return false;
		}
	}
	if (token != TK_IntConst && token != TK_FloatConst) 
	{
		return false;
	}
	if (neg)
	{
		number = -number;
		decimal = -decimal;
	}
	return true;
}

bool Scanner::CheckInteger() 
{ 
	Scanner savedstate;
	SaveState(savedstate);
	bool res = ScanInteger();
	if (!res) RestoreState(savedstate);
	return res;
}

bool Scanner::CheckFloat()
{
	Scanner savedstate;
	SaveState(savedstate);
	bool res = ScanFloat();
	if (!res) RestoreState(savedstate);
	return res;
}

void Scanner::MustGetInteger()
{
	if (!ScanInteger()) Error(TK_IntConst);
}

void Scanner::MustGetFloat()
{
	if (!ScanFloat()) Error(TK_FloatConst);
}


bool Scanner::TokensLeft() const
{
	return scanPos < length;
}

// This is taken from ZDoom's strbin function which can do a lot more than just unescaping backslashes and quotation marks.
void Scanner::Unescape(char *str)
{
	char *start = str;
	char *p = str, c;
	int i;

	while ((c = *p++)) {
		if (c != '\\') {
			*str++ = c;
		}
		else {
			switch (*p) {
			case 'a':
				*str++ = '\a';
				break;
			case 'b':
				*str++ = '\b';
				break;
			case 'f':
				*str++ = '\f';
				break;
			case 'n':
				*str++ = '\n';
				break;
			case 't':
				*str++ = '\t';
				break;
			case 'r':
				*str++ = '\r';
				break;
			case 'v':
				*str++ = '\v';
				break;
			case '?':
				*str++ = '\?';
				break;
			case '\n':
				break;
			case 'x':
			case 'X':
				c = 0;
				for (i = 0; i < 2; i++)
				{
					p++;
					if (*p >= '0' && *p <= '9')
						c = (c << 4) + *p - '0';
					else if (*p >= 'a' && *p <= 'f')
						c = (c << 4) + 10 + *p - 'a';
					else if (*p >= 'A' && *p <= 'F')
						c = (c << 4) + 10 + *p - 'A';
					else
					{
						p--;
						break;
					}
				}
				*str++ = c;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				c = *p - '0';
				for (i = 0; i < 2; i++)
				{
					p++;
					if (*p >= '0' && *p <= '7')
						c = (c << 3) + *p - '0';
					else
					{
						p--;
						break;
					}
				}
				*str++ = c;
				break;
			default:
				*str++ = *p;
				break;
			}
			p++;
		}
	}
	*str = 0;
}

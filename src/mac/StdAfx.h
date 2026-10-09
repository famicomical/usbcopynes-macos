/* macOS stand-in for ../StdAfx.h. Force-included with -include; it defines the
   Windows header's include guard so the original becomes a no-op and the c_*.c
   command files compile unchanged. */
#ifndef AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/syslimits.h>

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef uint32_t DWORD;
typedef uint32_t UINT32;
typedef void *HWND;
typedef void *HINSTANCE;

#define TRUE 1
#define FALSE 0
#define MAX_PATH PATH_MAX
#define __cdecl

/* MessageBox flags/results, Windows values */
#define MB_OK			0x0
#define MB_YESNOCANCEL		0x3
#define MB_YESNO		0x4
#define MB_ICONERROR		0x10
#define MB_ICONQUESTION		0x20
#define MB_ICONWARNING		0x30
#define MB_ICONINFORMATION	0x40
#define IDOK		1
#define IDCANCEL	2
#define IDYES		6
#define IDNO		7

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define strnicmp strncasecmp
#define stricmp strcasecmp
#define _vsnprintf vsnprintf
#define _snprintf snprintf

/* implemented in shim.c */
int	MessageBox (HWND hWnd, const char *text, const char *title, unsigned flags);
void	Sleep (unsigned ms);
char *	_fullpath (char *abspath, const char *relpath, size_t maxlen);
int	GetModuleFileName (void *module, char *buf, int len);
int	GetPrivateProfileInt (const char *sec, const char *key, int def, const char *file);
int	GetPrivateProfileString (const char *sec, const char *key, const char *def, char *out, int len, const char *file);
int	WritePrivateProfileString (const char *sec, const char *key, const char *val, const char *file);
void	ListPlugins (int Type);
void	SetAnswers (char **answers, int count);

#include "../CopyNESW.h"
#include "../version.h"

#endif

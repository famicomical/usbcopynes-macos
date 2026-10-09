/* Console replacements for the Win32 dialogs in miscdialogs.c and the handful
   of Win32 API calls the command files use. */
#include "StdAfx.h"
#include <poll.h>
#include <mach-o/dyld.h>

HINSTANCE hInst;
HWND topHWnd;

/* Answers given on the command line are consumed by prompts in order; after
   they run out, prompts read stdin. */
static char **answers;
static int nanswers;
void	SetAnswers (char **a, int n)
{
	answers = a;
	nanswers = n;
}

static const char *nextAnswer (void)
{
	if (!nanswers)
		return NULL;
	nanswers--;
	return *answers++;
}

static int percentLine;	/* last stderr output was a \r percent, needs \n before text */
static void	endPercent (void)
{
	if (percentLine)
		fputc('\n', stderr);
	percentLine = 0;
}

static BOOL	ask (const char *title, char *out, int len)
{
	const char *a = nextAnswer();
	endPercent();
	if (a)
	{
		snprintf(out, len, "%s", a);
		fprintf(stderr, "%s: %s\n", title, a);
		return out[0] != 0;
	}
	fprintf(stderr, "%s\n> ", title);
	if (!fgets(out, len, stdin))
	{
		out[0] = 0;
		return FALSE;
	}
	out[strcspn(out, "\r\n")] = 0;
	return out[0] != 0;
}

int	MessageBox (HWND hWnd, const char *text, const char *title, unsigned flags)
{
	char buf[16];
	endPercent();
	if ((flags & 0xF) == MB_OK)
	{
		fprintf(stderr, "%s: %s\n", title, text);
		return IDOK;
	}
	for (;;)
	{
		const char *a = nextAnswer();
		if (a)
		{
			snprintf(buf, sizeof buf, "%s", a);
			fprintf(stderr, "%s: %s %s\n", title, text, a);
		}
		else
		{
			fprintf(stderr, "%s: %s [y/n] ", title, text);
			if (!fgets(buf, sizeof buf, stdin))
				return (flags & 0xF) == MB_YESNOCANCEL ? IDCANCEL : IDNO;
		}
		if (tolower(buf[0]) == 'y')
			return IDYES;
		if (tolower(buf[0]) == 'n')
			return IDNO;
		if ((flags & 0xF) == MB_YESNOCANCEL)
			return IDCANCEL;
	}
}

void	Sleep (unsigned ms)
{
	usleep(ms * 1000);
}

/* Status window */
void	OpenStatus (HWND hWnd) {}
void	CloseStatus (void) {}
void	DoEvents (void) {}

void	__cdecl	StatusText (char *text, ...)
{
	va_list ap;
	endPercent();
	va_start(ap, text);
	vfprintf(stderr, text, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void	StatusPercent (int percent)
{
	fprintf(stderr, "\r%3d%%", percent);
	percentLine = 1;
}

static BOOL stdinReady (void)
{
	struct pollfd p = { 0, POLLIN, 0 };
	return poll(&p, 1, 0) > 0;
}

void	StatusButton (void)
{
	char buf[256];
	endPercent();
	if (!isatty(0))
		return;
	fprintf(stderr, "Press Enter to continue...");
	if (!fgets(buf, sizeof buf, stdin))
		clearerr(stdin);
	fputc('\n', stderr);
}

void	StatusButtonAsync (BOOL enabled)
{
	endPercent();
	if (enabled)
		fprintf(stderr, "Press Enter to stop.\n");
}

BOOL	StatusButtonPressed (void)
{
	char buf[256];
	if (!stdinReady())
		return FALSE;
	fgets(buf, sizeof buf, stdin);
	return TRUE;
}

void	StatusOK (void)
{
	StatusButton();
}

/* Text prompt */
BOOL	Prompt (HWND hWnd)
{
	return ask(PromptTitle, PromptResult, sizeof PromptResult);
}

BOOL	PromptLong (HWND hWnd)
{
	return ask(PromptTitle, PromptResult, sizeof PromptResult);
}

/* File prompt. Relative names that don't exist as given are tried in InitDir. */
BOOL	PromptFile (HWND hWnd, char *Filter, char *FilePath, char *FileName, char *InitDir, char *Title, char *DefExt, BOOL Save)
{
	char buf[MAX_PATH];
	if (!ask(Title, buf, sizeof buf))
		return FALSE;
	if (buf[0] != '/' && InitDir && access(buf, F_OK) != 0)
		snprintf(FilePath, MAX_PATH, "%s%s", InitDir, buf);
	else
		strcpy(FilePath, buf);
	if (!Save && access(FilePath, F_OK) != 0)
	{
		fprintf(stderr, "File not found: %s\n", FilePath);
		return FALSE;
	}
	if (FileName)
	{
		const char *b = strrchr(FilePath, '/');
		strcpy(FileName, b ? b + 1 : FilePath);
	}
	return TRUE;
}

/* Board name prompt */
void	ListPlugins (int Type)
{
	int i, j;
	for (i = 0; i <= numcats; i++)
	{
		if (Type != -1 && Plugins[i]->type != Type)
			continue;
		fprintf(stderr, "\n== %s\n", Plugins[i]->desc);
		for (j = 0; j < Plugins[i]->listlen; j++)
			fprintf(stderr, "  %-26s %-12s %4d  %s\n", Plugins[i]->list[j]->name, Plugins[i]->list[j]->file,
				Plugins[i]->list[j]->num, Plugins[i]->list[j]->desc);
	}
}

static char custplug_name[1024] = "CUSTOM";
static char custplug_file[1024] = "custom.bin";
static char custplug_desc[1024] = "User-Defined Mapper";
static TPlugin custplug = {custplug_name, custplug_file, 9998, custplug_desc, 0};

PPlugin	PromptPlugin (int Type)
{
	char buf[256], filepath[MAX_PATH];
	PPlugin result = NULL;
	int i, j;

	while (!result)
	{
		if (!ask("Plugin name or .bin file (? lists)", buf, sizeof buf))
			return NULL;
		if (!strcmp(buf, "?"))
		{
			ListPlugins(Type);
			continue;
		}
		for (i = 0; i <= numcats && !result; i++)
			if (Plugins[i]->type == Type)
				for (j = 0; j < Plugins[i]->listlen; j++)
					if (!strcasecmp(Plugins[i]->list[j]->name, buf) || !strcasecmp(Plugins[i]->list[j]->file, buf))
					{
						result = Plugins[i]->list[j];
						break;
					}
		if (!result)
		{
			/* a .bin not listed in MAPPERS.DAT: run it as a custom plugin */
			snprintf(filepath, sizeof filepath, "%s%s", Path_PLUG, buf);
			if (strlen(buf) > 4 && !strcasecmp(buf + strlen(buf) - 4, ".bin") && access(filepath, F_OK) == 0)
			{
				strcpy(custplug.file, buf);
				result = &custplug;
			}
			else
				fprintf(stderr, "No plugin named '%s' in this category.\n", buf);
		}
	}
	if (result->num != 9998)
		return result;

	/* user-defined mapper, same questions as the Windows client */
	PromptTitle = "Specify iNES mapper number (cancel for none):\n(-1 to skip .NES, 9999 to skip .NES and .UNIF)";
	if (Prompt(topHWnd))
		custplug.num = atoi(PromptResult);
	else	custplug.num = -1;

	if (custplug.num != 9999)
	{
		PromptTitle = "Specify UNIF board name (cancel for none):";
		if (Prompt(topHWnd))
			strcpy(custplug.name,PromptResult);
		else if (custplug.num == -1)
			custplug.num = 9999;
	}

	if (result == &custplug)
		return &custplug;	/* file already chosen above */
	while (1)
	{
		if (!PromptFile(topHWnd,"USB CopyNES Plugins (*.bin)", filepath, custplug.file, Path_PLUG, "Select a plugin (must be in Plugins path)", "bin", FALSE))
			return NULL;
		if (strnicmp(Path_PLUG, filepath, strlen(Path_PLUG)))
			MessageBox(topHWnd,"Selected file is not located in Plugins directory!","Select Plugin", MB_OK | MB_ICONERROR);
		else	break;
	}
	return &custplug;
}

/* Win32 API used by config.c */
char *	_fullpath (char *abspath, const char *relpath, size_t maxlen)
{
	if (relpath[0] == '/')
		snprintf(abspath, maxlen, "%s", relpath);
	else
		snprintf(abspath, maxlen, "%s%s", Path_MAIN, relpath);
	return abspath;
}

int	GetModuleFileName (void *module, char *buf, int len)
{
	char exe[MAX_PATH];
	uint32_t n = sizeof exe;
	if (_NSGetExecutablePath(exe, &n) != 0 || !realpath(exe, buf))
		return 0;
	return strlen(buf);
}

/* shortcut: single-section ini, section name ignored, upgrade if a second section ever appears */
int	GetPrivateProfileString (const char *sec, const char *key, const char *def, char *out, int len, const char *file)
{
	char line[1024], *p;
	size_t klen = strlen(key);
	FILE *f = fopen(file, "r");
	snprintf(out, len, "%s", def);
	if (!f)
		return strlen(out);
	while (fgets(line, sizeof line, f))
	{
		if (strncmp(line, key, klen) || line[klen] != '=')
			continue;
		line[strcspn(line, "\r\n")] = 0;
		snprintf(out, len, "%s", line + klen + 1);
		for (p = out; *p; p++)	/* ini written by the Windows client uses backslashes */
			if (*p == '\\')
				*p = '/';
		break;
	}
	fclose(f);
	return strlen(out);
}

int	GetPrivateProfileInt (const char *sec, const char *key, int def, const char *file)
{
	char buf[32];
	GetPrivateProfileString(sec, key, "", buf, sizeof buf, file);
	return buf[0] ? atoi(buf) : def;
}

/* shortcut: config is read-only on macOS, upgrade when the options command is ported */
int	WritePrivateProfileString (const char *sec, const char *key, const char *val, const char *file)
{
	return 1;
}

#include "StdAfx.h"

BOOL	Startup (void);
void	Shutdown (void);

BOOL	CMD_MAKENES (void);	BOOL	CMD_MAKEUNIF (void);	BOOL	CMD_SPLITNES (void);	BOOL	CMD_SPLITUNIF (void);
BOOL	CMD_BREAKBANK (void);	BOOL	CMD_DISASM (void);	BOOL	CMD_NESINFO (void);
BOOL	CMD_PLAYCART (void);	BOOL	CMD_RAMCART (void);	BOOL	CMD_PLAYLOG (void);
BOOL	CMD_DUMPCART (void);	BOOL	CMD_WRITEWRAM (void);	BOOL	CMD_RUNPLUG (void);	BOOL	CMD_FIXGAR (void);

static BOOL	CMD_LIST (void)
{
	ListPlugins(-1);
	return TRUE;
}

static BOOL	CMD_CONFIG (void)
{
	printf("Path_MAIN=%s\nPath_PRG=%s\nPath_CHR=%s\nPath_WRAM=%s\nPath_NES=%s\nPath_CRC=%s\nPath_NSF=%s\nPath_PLUG=%s\n"
		"SaveCRC=%d SaveFiles=%d MakeUnif=%d HWVer=%d\n",
		Path_MAIN, Path_PRG, Path_CHR, Path_WRAM, Path_NES, Path_CRC, Path_NSF, Path_PLUG, SaveCRC, SaveFiles, MakeUnif, HWVer);
	return TRUE;
}

static const struct
{
	const char *name;
	BOOL (*fn) (void);
	BOOL online;	/* needs a connected CopyNES */
	const char *help;
}	cmds[] =
{
	{"dump",	CMD_DUMPCART,	TRUE,	"[plugin] [romname]   dump cartridge"},
	{"playcart",	CMD_PLAYCART,	TRUE,	"                     reset into play mode"},
	{"runplug",	CMD_RUNPLUG,	TRUE,	"[plugin]             run a utility plugin"},
	{"writewram",	CMD_WRITEWRAM,	TRUE,	"[plugin] [file.sav]  upload WRAM"},
	{"ramcart",	CMD_RAMCART,	TRUE,	"[plugin] [file.nes]  program RAM/flash cart"},
	{"fixgar",	CMD_FIXGAR,	TRUE,	"                     fix garbled Game Genie"},
	{"nesinfo",	CMD_NESINFO,	TRUE,	"                     show NES hardware info"},
	{"playlog",	CMD_PLAYLOG,	TRUE,	"[file.txt]           play a register log"},
	{"makenes",	CMD_MAKENES,	FALSE,	"[romname] ...        build .nes from PRG/CHR"},
	{"makeunif",	CMD_MAKEUNIF,	FALSE,	"[romname] ...        build .unf from PRG/CHR"},
	{"splitnes",	CMD_SPLITNES,	FALSE,	"[file.nes]           split .nes into PRG/CHR"},
	{"splitunif",	CMD_SPLITUNIF,	FALSE,	"[file.unf]           split .unf into PRG/CHR"},
	{"breakbank",	CMD_BREAKBANK,	FALSE,	"[file] ...           split a file into banks"},
	{"disasm",	CMD_DISASM,	FALSE,	"[file] ...           disassemble 6502"},
	{"list",	CMD_LIST,	FALSE,	"                     list plugins from MAPPERS.DAT"},
	{"config",	CMD_CONFIG,	FALSE,	"                     show resolved paths and BIOS version"},
};

int main (int argc, char **argv)
{
	int i;
	BOOL ok;

	for (i = 0; i < (int)(sizeof cmds / sizeof *cmds); i++)
		if (argc >= 2 && !strcmp(argv[1], cmds[i].name))
			break;
	if (i == (int)(sizeof cmds / sizeof *cmds))
	{
		fprintf(stderr, "%s\nusage: copynes <command> [answers...]\n"
			"Answers are consumed by prompts in order; omit them to be asked interactively.\n"
			"[plugin] is a MAPPERS.DAT name, its .bin filename, or any .bin in PLUGINS/ (asks mapper number).\n\n", APPTITLE);
		for (i = 0; i < (int)(sizeof cmds / sizeof *cmds); i++)
			fprintf(stderr, "  %-10s %s\n", cmds[i].name, cmds[i].help);
		return 2;
	}
	SetAnswers(argv + 2, argc - 2);

	if (!Startup())	/* config, MAPPERS.DAT, USB open, BIOS version */
		return 1;
	ParPort = -1;	/* only USB exists here; stale ini values would mislabel UNIF dumper info */
	if (cmds[i].online && HWVer == 0)
	{
		fprintf(stderr, "CopyNES not connected.\n");
		Shutdown();
		return 1;
	}
	ok = cmds[i].fn();
	Shutdown();
	return ok ? 0 : 1;
}

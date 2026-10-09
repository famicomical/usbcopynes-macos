/* USB half of ../io.cpp on libftdi. The CopyNES is an FT2232: interface A is
   the data bus, interface B's DTR/RTS drive /RESET and play/copy mode. The
   parallel port path is not ported. */
#include "StdAfx.h"
#include <ftdi.h>

BOOL connection_initialized = FALSE;
char	ROMstring[256];

static struct ftdi_context ftA, ftB;
static BOOL portOpen = FALSE;
static int stash = -1;	/* byte pulled off the wire by ReadByteReady, handed back by ReadByteEx */

static BOOL	openChannel (struct ftdi_context *ctx, enum ftdi_interface iface, const char *what)
{
	if (ftdi_init(ctx) < 0)
	{
		MessageBox(topHWnd, "USB Error: libftdi init failed!", "OpenUSB", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	ftdi_set_interface(ctx, iface);
	/* shortcut: stock FT2232 VID/PID hardcoded, make it configurable if a CopyNES turns up with another PID */
	if (ftdi_usb_open_desc(ctx, 0x0403, 0x6010, "USB CopyNES", NULL) < 0)
	{
		fprintf(stderr, "USB Error: Failed to open CopyNES %s bus! (%s)\n", what, ftdi_get_error_string(ctx));
		ftdi_deinit(ctx);
		return FALSE;
	}
	return TRUE;
}

BOOL	OpenPort (int port, int addr, int ecp)
{
	if (!openChannel(&ftA, INTERFACE_A, "data"))
		return FALSE;
	if (!openChannel(&ftB, INTERFACE_B, "control"))
	{
		ftdi_usb_close(&ftA);
		ftdi_deinit(&ftA);
		return FALSE;
	}
	ftdi_set_latency_timer(&ftA, 2);	/* single-byte reads poll the chip; default 16ms is slow */
	ftA.usb_read_timeout = 100;		/* ReadByteEx loops on its own deadline */
	portOpen = TRUE;
	connection_initialized = TRUE;
	return TRUE;
}

void	ClosePort (void)
{
	if (!portOpen)
		return;
	ftdi_usb_close(&ftA);
	ftdi_deinit(&ftA);
	ftdi_usb_close(&ftB);
	ftdi_deinit(&ftB);
	portOpen = FALSE;
	connection_initialized = FALSE;
}

/* 1 = got byte, 0 = timeout, -1 = error */
static int	usbRead (BYTE *data, int timeout)
{
	time_t start = time(NULL);
	if (stash >= 0)
	{
		*data = (BYTE)stash;
		stash = -1;
		return 1;
	}
	for (;;)
	{
		int n = ftdi_read_data(&ftA, data, 1);
		if (n == 1)
			return 1;
		if (n < 0)
			return -1;
		if (time(NULL) > start + timeout)
			return 0;
		usleep(1000);
	}
}

BOOL	ReadByteEx (BYTE *data, int timeout, BOOL warn)
{
	int r;
	if (data == NULL || !portOpen)
		return FALSE;
	r = usbRead(data, timeout);
	if (r == 1)
		return TRUE;
	if (warn)
		MessageBox(topHWnd, r == 0 ? "USB Error: Read Timeout" : "USB Error: Read Failed", "ReadByteEx", MB_OK | MB_ICONERROR);
	return FALSE;
}

BOOL	WriteByteEx (BYTE data, int timeout, BOOL warn)
{
	if (!portOpen)
		return FALSE;
	ftA.usb_write_timeout = timeout * 1000;
	if (ftdi_write_data(&ftA, &data, 1) == 1)
		return TRUE;
	if (warn)
		MessageBox(topHWnd, "USB Error: Write Failed", "WriteByteEx", MB_OK | MB_ICONERROR);
	return FALSE;
}

BOOL	WriteBlock (BYTE *blockdata, int size)
{
	if (!portOpen)
		return FALSE;
	ftA.usb_write_timeout = 10000;
	if (ftdi_write_data(&ftA, blockdata, size) == size)
		return TRUE;
	MessageBox(topHWnd, "USB Error: Write Failed", "WriteBlock", MB_OK | MB_ICONERROR);
	return FALSE;
}

BOOL	ReadByte (BYTE *data)		{ return ReadByteEx(data, 10, TRUE); }
BOOL	WriteByte (BYTE data)		{ return WriteByteEx(data, 10, TRUE); }
BOOL	ReadByteSilent (BYTE *data)	{ return ReadByteEx(data, 10, FALSE); }
BOOL	WriteByteSilent (BYTE data)	{ return WriteByteEx(data, 10, FALSE); }

BOOL	ReadByteReady (void)
{
	BYTE b;
	if (!portOpen)
		return FALSE;
	if (stash >= 0)
		return TRUE;
	if (ftdi_read_data(&ftA, &b, 1) == 1)
	{
		stash = b;
		return TRUE;
	}
	return FALSE;
}

void	InitPort (void)
{
	unsigned short modem;
	if (!portOpen)
		return;
	if (ftdi_poll_modem_status(&ftB, &modem) < 0)
	{
		MessageBox(topHWnd, "USB Error: Get Power Status Failed!", "InitPort", MB_OK | MB_ICONERROR);
		return;
	}
	if (modem & 0x80)	/* DCD */
	{
		MessageBox(topHWnd, "USB CopyNES is OFF - turn on then hit OK", "InitPort", MB_OK | MB_ICONERROR);
		StatusButton();
	}
	stash = -1;
	if (ftdi_tcioflush(&ftA) < 0 || ftdi_tcioflush(&ftB) < 0)
	{
		MessageBox(topHWnd, "USB Error: Purge Failed!", "InitPort", MB_OK | MB_ICONERROR);
		return;
	}
	connection_initialized = TRUE;
}

void	ResetNES (int rtype)
{
	if (!portOpen)
		return;
	/* RTS asserted = copy mode, released = play mode */
	if (ftdi_setrts(&ftB, (rtype & RESET_PLAYMODE) ? 0 : 1) < 0)
	{
		MessageBox(topHWnd, "USB Error: SetRts Failed!", "ResetNES", MB_OK | MB_ICONERROR);
		return;
	}
	if (!(rtype & RESET_NORESET))
	{
		if (ftdi_setdtr(&ftB, 0) < 0)	/* pull /RESET low */
		{
			MessageBox(topHWnd, "USB Error: ClrDtr Failed!", "ResetNES", MB_OK | MB_ICONERROR);
			return;
		}
		Sleep(SLEEP_SHORT);
	}
	if (ftdi_setdtr(&ftB, 1) < 0)	/* pull /RESET high */
	{
		MessageBox(topHWnd, "USB Error: SetDtr Failed!", "ResetNES", MB_OK | MB_ICONERROR);
		return;
	}
	Sleep(SLEEP_SHORT);
	InitPort();
	Sleep(SLEEP_SHORT);
}

BOOL	WriteCommand (BYTE a, BYTE b, BYTE c, BYTE d, BYTE e)
{
	if (WriteByteSilent(a) && WriteByteSilent(b) && WriteByteSilent(c) && WriteByteSilent(d) && WriteByteSilent(e))
		return TRUE;
	MessageBox(topHWnd, "USB Error: Timeout on data transfer!", "WriteCommand", MB_OK | MB_ICONERROR);
	return FALSE;
}

BOOL	LoadPlugin (char *plugin)
{
	int w;
	char filename[MAX_PATH];
	FILE *PLUGIN;
	char *filedata;
	BOOL status;

	strcpy(filename,Path_PLUG);
	strcat(filename,plugin);

	if ((PLUGIN = fopen(filename,"rb")) == NULL)
	{
		MessageBox(topHWnd,"Failed to open plugin file!","LoadPlugin",MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (!WriteCommand(0x4B,0x00,0x04,0x04,0xB4))	// write to CPU space
	{
		fclose(PLUGIN);
		return FALSE;
	}
	fseek(PLUGIN,128,SEEK_SET);
	filedata = (char*)malloc(1024);
	fread((void*)filedata, 1024, 1, PLUGIN);

	status = WriteBlock((BYTE*)filedata, 1024);
	free(filedata);

	if (!status)
	{
		fclose(PLUGIN);
		return FALSE;
	}

	for (w = 0; !feof(PLUGIN) && w < (int)sizeof(ROMstring) - 1; w++)
		fread(&ROMstring[w],1,1,PLUGIN);
	ROMstring[w] = 0;
	fclose(PLUGIN);
	StatusPercent(0);
	Sleep(SLEEP_SHORT);
	return TRUE;
}

BOOL	RunCode (void)
{
	return WriteCommand(0x7E,0x00,0x04,0x00,0xE7);
}

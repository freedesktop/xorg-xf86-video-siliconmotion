/* Header:   //Mercury/Projects/archives/XFree86/4.0/smi.h-arc   1.51   29 Nov 2000 17:45:16   Frido  $ */

/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.
Copyright (C) 2000 Silicon Motion, Inc.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the XFree86 Project and
Silicon Motion shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the XFree86 Project and Silicon Motion.
*/

#ifndef _SMI_H
#define _SMI_H

#include "smi_pcirename.h"

#include <string.h>
#include <stdio.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86Cursor.h"
#include "vgaHW.h"

#include "compiler.h"

#include "mipointer.h"
#include "micmap.h"

#include "fb.h"

#include "xaa.h"
#include "exa.h"
#include "xf86cmap.h"
#include "xf86i2c.h"

#include "xf86int10.h"
#include "vbe.h"

#include "xf86xv.h"
#include <X11/extensions/Xv.h>

/******************************************************************************/
/*			D E F I N I T I O N S				      */
/******************************************************************************/

#ifndef SMI_DEBUG
#define SMI_DEBUG		0
#endif

#define SMI_USE_IMAGE_WRITES	0
#define SMI_USE_VIDEO		1
#define SMI_USE_CAPTURE		1

/*
 *   Leaving attempt implementation of an argb cursor using alpha plane
 * for the smi 501/502 under this ifdef for now. Maybe it will be fixed
 * in a subsequent hardware revision.
 *   The problem is that the alpha plane will only work (that is, become
 * visible) if alpha_plane_tl is set to top:=0 and left:=0.
 *   Also, if alpha_plane_br does not match panel dimensions, the alpha
 * plane will be displayed tilled in the "first" row, with corruption on
 * on all odd columns.
 *   Since setting the alpha fb_address works, to implement an argb cursor
 * using the alpha plane, with the current hardware bugs, it would be
 * required to:
 *	o allocate an offscreen area of pSmi->lcdWidth * pSmi->lcdHeight * 2
 *	o set statically tl/tr to 0,0,pSmi->lcdWidth-1,pSmi->lcdHeight-1
 *	o use alpha format 3 (argb 4:4:4:4), or alternatively format 1
 *	  (rgb 5:6:5), and in the last case, a global 50% alpha is the
 *	  best bet, and for the argb cursors being used at the time of this
 *	  writing, they look correct, while 100% opaque looks wrong.
 *	o when positioning the pointer, first erase it from the offscreen
 *	  area, then repaint it at the proper offset in the alpha offscreen
 *	  area.
 *  .... argb software cursor works way better
 *   (There are some other alternatives, like using 8 bits indexed mode,
 * but when using a global alpha value, instead of per pixel, most argb
 * cursors will not look correctly, regardless of the alpha value, that
 * should be from 50 to 100% transparency).
 *   But still there would be the problem of memory requiring a 128 bit
 * alignment, what would require either moving the image in the memory,
 * and/or some trick with the vsync pixel panning.
 *
 *   Until the alpha layer is corrected in some newer revision (or removed?),
 * it could be used as something like an alternate crt, that happens to be
 * on top of the panel (and has 16 transparency levels).
 */
#define SMI_CURSOR_ALPHA_PLANE	0

/******************************************************************************/
/*			S T R U C T U R E S				      */
/******************************************************************************/

/* Driver data structure; this should contain all needed info for a mode */
typedef struct
{
    Bool    modeInit;
    CARD16	mode;
    CARD8	SR17, SR18, SR21, SR31, SR32, SR6A, SR6B, SR81, SRA0;
    CARD8	CR33, CR33_2, CR3A;
    CARD8	CR40[14], CR40_2[14];
    CARD8	CR90[16], CR9F_2;
    CARD8	CRA0[14];
    CARD8	smiDACMask, smiDacRegs[256][3];
    /* CZ 2.11.2001: for gamma correction */
    CARD8   CCR66;
    /* end CZ */
    CARD8	smiFont[8192];
    CARD32	DPR10, DPR1C, DPR20, DPR24, DPR28, DPR2C, DPR30, DPR3C, DPR40,
		DPR44;
    CARD32	VPR00, VPR0C, VPR10;
    CARD32	CPR00;
    CARD32	FPR00_, FPR0C_, FPR10_;
    /* LCD FIFO regs, etc. - dualhead */
    CARD8   SR22, SR40, SR41, SR42, SR43, SR44, SR45, SR48, SR49,
	    SR4A, SR4B, SR4C;
    /* PLL controls */
    CARD8   SR68, SR69, SR6C, SR6D, SR6E, SR6F;
} SMIRegRec, *SMIRegPtr;

/* Global PDEV structure. */
typedef struct
{
    int			Bpp;		/* Bytes per pixel */
    int			MCLK;		/* Memory Clock  */
    ClockRanges		clockRange;	/* Allowed pixel clock range */
    CloseScreenProcPtr	CloseScreen;	/* Pointer used to save wrapped
					   CloseScreen function */

    I2CBusPtr		I2C;		/* Pointer into I2C module */
    xf86Int10InfoPtr	pInt10;		/* Pointer to INT10 module */
    vbeInfoPtr          pVbe;           /* Pointer to VBE module */

    pciVideoPtr		PciInfo;	/* PCI info vars */
#ifndef XSERVER_LIBPCIACCESS
    PCITAG		PciTag;
#endif
    int			Chipset;	/* Chip info, set using PCI
					   above */
    int			ChipRev;

    OptionInfoPtr	Options;
    Bool		Dualhead;

    /* Don't attempt to program a video mode. Use kernel framebuffer
     * mode instead. */
    Bool		UseFBDev;

    /* CSC video uses color space conversion to render video directly to
     * the framebuffer, without using an overlay. */
    Bool		CSCVideo;

    Bool		PCIBurst;	/* Enable PCI burst mode for
					   reads? */
    Bool		PCIRetry;	/* Enable PCI retries */
    Bool		HwCursor;	/* hardware cursor enabled */

    CARD8		DACmask;
    int			vgaCRIndex, vgaCRReg;
    Bool		PrimaryVidMapped;	/* Flag indicating if
						   vgaHWMapMem was used
						   successfully for
						   this screen */
    Bool		ModeStructInit;	/* Flag indicating ModeReg has
					   been duped from console
					   state */

    /* Hardware state */
    void		(*Save)(ScrnInfoPtr pScrn); /* Function used to save the
						       current register state */
    CARD8		SR18Value;	/* PDR#521: original SR18
					   value */
    CARD8		SR21Value;	/* PDR#521: original SR21
					   value */
    void		*save;		/* console saved mode
					   registers */
    void		*mode;		/* XServer video state mode
					   registers */

    /* Memory layout */
    int			videoRAMBytes;	/* In units as noted, set in
					   PreInit  */
    int			videoRAMKBytes;	/* In units as noted, set in
					   PreInit */
    unsigned char *	MapBase;	/* Base of mapped memory */
    int			MapSize;	/* Size of mapped memory */
    CARD8 *		DPRBase;	/* Base of DPR registers */
    CARD8 *		VPRBase;	/* Base of VPR registers */
    CARD8 *		CPRBase;	/* Base of CPR registers */
    CARD8 *		FPRBase;    /* Base of FPR registers - for 0730 chipset */
    CARD8 *		DCRBase;		/* Base of DCR registers - for 0501 chipset */
    CARD8 *		SCRBase;        /* Base of SCR registers - for 0501 chipset */
    CARD8 *		DataPortBase;	/* Base of data port */
    int			DataPortSize;	/* Size of data port */
    CARD8 *		IOBase;		/* Base of MMIO VGA ports */
    IOADDRESS		PIOBase;	/* Base of I/O ports */
    unsigned char *	FBBase;		/* Base of FB */
    CARD32		fbMapOffset;    /* offset for fb mapping */
    CARD32		FBOffset;	/* Current visual FB starting
					   location */
    CARD32		FBCursorOffset;	/* Cursor storage location */
    CARD32		FBReserved;	/* Reserved memory in frame
					   buffer */

    /* accel additions */
    CARD32		AccelCmd;	/* Value for DPR0C */
    Bool		NoAccel;	/* Disable Acceleration */
    CARD32		ScissorsLeft;	/* Left/top of current
					   scissors */
    CARD32		ScissorsRight;	/* Right/bottom of current
					   scissors */
    Bool		ClipTurnedOn;	/* Clipping was turned on by
					   the previous command */
    int			GEResetCnt;	/* Limit the number of errors
					   printed using a counter */

    Bool		useBIOS;	/* Use BIOS for mode sets */
    XAAInfoRecPtr	XAAInfoRec;	/* XAA info Rec */

    /* EXA */
    ExaDriverPtr	EXADriverPtr;
    Bool		useEXA;		/* enable exa acceleration */
    ExaOffscreenArea*	fbArea;		/* EXA offscreen area used
					   as framebuffer */
    PictTransformPtr	renderTransform;

    /* DGA */
    DGAModePtr		DGAModes;	/* Pointer to DGA modes */
    int			numDGAModes;	/* Number of DGA modes */
    Bool		DGAactive;	/* Flag if DGA is active */
    int			DGAViewportStatus;

    /* DPMS */
    int			CurrentDPMS;	/* Current DPMS state */

    /* Panel information */
    Bool		lcd;		/* LCD active, 1=DSTN, 2=TFT */
    int			lcdWidth;	/* LCD width */
    int			lcdHeight;	/* LCD height */

#if 0
    /* Polylines - #671 */
    ValidateGCProcPtr	ValidatePolylines;	/* Org.
						   ValidatePolylines
						   function */
    Bool		polyLines;	/* Our polylines patch is
					   active */
#endif

    /* XvExtension */
    int			videoKey;	/* Video chroma key */
    Bool		ByteSwap;	/* Byte swap for ZV port */
    Bool		interlaced;	/* True: Interlaced Video */
    XF86VideoAdaptorPtr	ptrAdaptor;	/* Pointer to VideoAdapter
					   structure */
    void (*BlockHandler)(int i, pointer blockData, pointer pTimeout,
					 pointer pReadMask);
} SMIRec, *SMIPtr;

#define SMIPTR(p) ((SMIPtr)((p)->driverPrivate))

/******************************************************************************/
/*			M A C R O S					      */
/******************************************************************************/

#if SMI_DEBUG
extern int smi_indent;
# define VERBLEV	1
# define ENTER()	xf86ErrorFVerb(VERBLEV, "%*c %s\n",\
				       smi_indent++, '>', __FUNCTION__)
# define LEAVE()	xf86ErrorFVerb(VERBLEV, "%*c %s\n",\
				       --smi_indent, '<', __FUNCTION__)
# define RETURN(value)							\
    do {								\
	xf86ErrorFVerb(VERBLEV, "%*c %s\n",				\
		       --smi_indent, '<', __FUNCTION__);		\
	return (value);							\
    } while (0)
# define DEBUG(...)	xf86ErrorFVerb(VERBLEV, __VA_ARGS__)
#else
# define VERBLEV	4
# define ENTER()	/**/
# define LEAVE()	/**/
# define RETURN(value)	return (value)
# define DEBUG(...)	/**/
#endif

/* Some Silicon Motion structs & registers */
#include "regsmi.h"

#if !defined (MetroLink) && !defined (VertDebug)
#define VerticalRetraceWait()						\
do									\
{									\
    if (VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x17) & 0x80)		\
    {									\
	while ((VGAIN8(pSmi, vgaIOBase + 0x0A) & 0x08) == 0x00);	\
	while ((VGAIN8(pSmi, vgaIOBase + 0x0A) & 0x08) == 0x08);	\
	while ((VGAIN8(pSmi, vgaIOBase + 0x0A) & 0x08) == 0x00);	\
    }									\
} while (0)
#else
#define SPIN_LIMIT 1000000
#define VerticalRetraceWait()						\
do									\
{									\
    if (VGAIN8_INDEX(pSmi, vgaCRIndex, vgaCRData, 0x17) & 0x80)		\
    {									\
	volatile unsigned long _spin_me;				\
	for (_spin_me = SPIN_LIMIT;					\
	     ((VGAIN8(pSmi, vgaIOBase + 0x0A) & 0x08) == 0x00) && 	\
	     _spin_me;							\
	     _spin_me--);						\
	if (!_spin_me)							\
	    ErrorF("smi: warning: VerticalRetraceWait timed out.\n");	\
	for (_spin_me = SPIN_LIMIT;					\
	     ((VGAIN8(pSmi, vgaIOBase + 0x0A) & 0x08) == 0x08) && 	\
	     _spin_me;							\
	     _spin_me--);						\
	if (!_spin_me)							\
	    ErrorF("smi: warning: VerticalRetraceWait timed out.\n");	\
	for (_spin_me = SPIN_LIMIT;					\
	     ((VGAIN8(pSmi, vgaIOBase + 0x0A) & 0x08) == 0x00) && 	\
	     _spin_me;							\
	     _spin_me--);						\
	if (!_spin_me)							\
	    ErrorF("smi: warning: VerticalRetraceWait timed out.\n");	\
	}								\
} while (0)
#endif

/******************************************************************************/
/*			F U N C T I O N   P R O T O T Y P E S		      */
/******************************************************************************/

/* smi_dac.c */
void SMI_CommonCalcClock(int scrnIndex, long freq, int min_m, int min_n1, 
			 int max_n1, int min_n2, int max_n2, long freq_min, 
			 long freq_max, unsigned char * mdiv, 
			 unsigned char * ndiv);

/* smi_i2c */
Bool SMI_I2CInit(ScrnInfoPtr pScrn);

/* smi_accel.c */
void SMI_AccelSync(ScrnInfoPtr pScrn);
void SMI_GEReset(ScrnInfoPtr pScrn, int from_timeout, int line, char *file);
void SMI_EngineReset(ScrnInfoPtr);
void SMI_SetClippingRectangle(ScrnInfoPtr, int, int, int, int);
void SMI_DisableClipping(ScrnInfoPtr);
CARD32 SMI_DEDataFormat(int bpp);

/* smi_xaa.c */
Bool SMI_XAAInit(ScreenPtr pScrn);

/* smi_exa.c */
Bool SMI_EXAInit(ScreenPtr pScrn);

/* smi_hwcurs.c */
Bool SMI_HWCursorInit(ScreenPtr pScrn);

/* smi_driver.c */
Bool SMI_MapMem(ScrnInfoPtr pScrn);
void SMI_UnmapMem(ScrnInfoPtr pScrn);
void SMI_AdjustFrame(int scrnIndex, int x, int y, int flags);
Bool SMI_SwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
void SMI_LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indicies,
		     LOCO *colors, VisualPtr pVisual);
xf86MonPtr SMI_ddc1(ScrnInfoPtr pScrn);
void SMI_PrintRegs(ScrnInfoPtr pScrn);

#ifndef HAVE_XMODES
/* smi_dga.c */
Bool SMI_DGAInit(ScreenPtr pScrn);
#endif

/* smi_video.c */
void SMI_InitVideo(ScreenPtr pScreen);

#endif  /*_SMI_H*/

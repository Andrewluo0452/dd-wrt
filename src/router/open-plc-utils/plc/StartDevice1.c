/*====================================================================*
 *
 *   Copyright (c) 2013 Qualcomm Atheros, Inc.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or 
 *   without modification, are permitted (subject to the limitations 
 *   in the disclaimer below) provided that the following conditions 
 *   are met:
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials 
 *     provided with the distribution.
 *
 *   * Neither the name of Qualcomm Atheros nor the names of 
 *     its contributors may be used to endorse or promote products 
 *     derived from this software without specific prior written 
 *     permission.
 *
 *   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE 
 *   GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
 *   COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
 *   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER 
 *   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 *   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   int StartDevice1 (struct plc * plc);
 *
 *   plc.h
 *
 *   This int6kboot plugin initialize a device having no NVRAM or blank
 *   or corrupted NVRAM; ensure Bootloader is running before starting;
 *   write SDRAM configuration then NVM and PIB files to SDRAM and
 *   start firmware execution;
 *
 *
 *   Contributor(s):
 *      Charles Maier <cmaier@qca.qualcomm.com>
 *
 *--------------------------------------------------------------------*/

#ifndef STARTDEVICE1_SOURCE
#define STARTDEVICE1_SOURCE

#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>

#include "../tools/error.h"
#include "../tools/files.h"
#include "../nvm/nvm.h"
#include "../pib/pib.h"
#include "../plc/plc.h"

int StartDevice1 (struct plc * plc)

{
	unsigned module = 0;
	struct nvm_header1 nvm_header;
	struct pib_header pib_header;
	uint32_t offset;
	if (WriteCFG (plc))
	{
		return (-1);
	}
	if (lseek (plc->NVM.file, 0, SEEK_SET))
	{
		error (PLC_EXIT (plc), errno, FILE_CANTHOME, plc->NVM.name);
		return (-1);
	}
	if (read (plc->NVM.file, &nvm_header, sizeof (nvm_header)) != sizeof (nvm_header))
	{
		error (PLC_EXIT (plc), errno, FILE_CANTREAD, plc->NVM.name);
		return (-1);
	}
	while (nvm_header.NEXTHEADER)
	{
		if (lseek (plc->NVM.file, LE32TOH (nvm_header.NEXTHEADER), SEEK_SET) == -1)
		{
			error (PLC_EXIT (plc), errno, FILE_CANTHOME, plc->NVM.name);
			return (-1);
		}
		if (read (plc->NVM.file, &nvm_header, sizeof (nvm_header)) != sizeof (nvm_header))
		{
			error (PLC_EXIT (plc), errno, FILE_CANTREAD, plc->NVM.name);
			return (-1);
		}
		module++;
	}
	if (lseek (plc->PIB.file, 0, SEEK_SET))
	{
		error (1, errno, FILE_CANTHOME, plc->PIB.name);
	}
	if (read (plc->PIB.file, &pib_header, sizeof (pib_header)) != sizeof (pib_header))
	{
		error (1, errno, FILE_CANTREAD, plc->PIB.name);
	}
	if (lseek (plc->PIB.file, 0, SEEK_SET))
	{
		error (1, errno, FILE_CANTHOME, plc->PIB.name);
	}
	if (BE16TOH (*(uint16_t *)(&pib_header)) < 0x0305)
	{
		offset = LEGACY_PIBOFFSET;
	}
	else if (BE16TOH (*(uint16_t *)(&pib_header)) < 0x0500)
	{
		offset = INT6x00_PIBOFFSET;
	}
	else
	{
		offset = AR7x00_PIBOFFSET;
	}
	if (WriteMEM (plc, &plc->PIB, 0, offset, LE16TOH (pib_header.PIBLENGTH)))
	{
		return (-1);
	}
	if (WriteFirmware1 (plc, module, &nvm_header))
	{
		return (-1);
	}
	if (StartFirmware1 (plc, module, &nvm_header))
	{
		return (-1);
	}
	if (lseek (plc->NVM.file, 0, SEEK_SET))
	{
		error (PLC_EXIT (plc), errno, FILE_CANTHOME, plc->NVM.name);
		return (-1);
	}
	return (0);
}


#endif


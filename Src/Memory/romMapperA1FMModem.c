/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperA1FMModem.c,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-30 18:38:42 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "romMapperA1FMModem.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "Board.h"
#include "Switches.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern UInt8 panasonicSramGet(UInt32 address);
extern void  panasonicSramSet(UInt32 address, UInt8 value);

typedef struct {
    int deviceHandle;
    UInt8* romData;
    int    romSize;
    int    slot;
    int    sslot;
    int    startPage;
    int    romMapper;
} RomMapperA1FMModem;

static void rommappera1fm_saveState(void *data)
{
    RomMapperA1FMModem *rm = (RomMapperA1FMModem*)data;
    SaveState* state = saveStateOpenForWrite("mapperPanasonicA1FM");

    saveStateSet(state, "romMapper", rm->romMapper);
    
    saveStateClose(state);
}

static void rommappera1fm_loadState(void *data)
{
    RomMapperA1FMModem *rm = (RomMapperA1FMModem*)data;
    SaveState* state       = saveStateOpenForRead("mapperPanasonicA1FM");
    rm->romMapper          = saveStateGet(state, "romMapper", 0);
    
    saveStateClose(state);
    
    slotMapPage(rm->slot, rm->sslot, rm->startPage, rm->romData + rm->romMapper * 0x2000, 1, 0);
}

static void rommappera1fm_destroy(void *data)
{
    RomMapperA1FMModem *rm = (RomMapperA1FMModem*)data;
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static UInt8 rommappera1fm_read(void *data, UInt16 address) 
{
    RomMapperA1FMModem* rm = (RomMapperA1FMModem*)data;
    address += 0x4000;

    if (address >= 0x7fc0 && address < 0x7fd0) {
		switch (address & 0x0f) {
		case 4:
			return rm->romMapper;
		case 6:
			return switchGetFront() ? 0xfb : 0xff;
		default:
			return 0xff;
		}
	} 

    return panasonicSramGet(address & 0x1fff);
}

static void rommappera1fm_write(void *data, UInt16 address, UInt8 value) 
{
    RomMapperA1FMModem *rm = (RomMapperA1FMModem*)data;
    address += 0x4000;

    if (address >= 0x6000 && address < 0x8000) {
        panasonicSramSet(address & 0x1fff, value);

		if (address == 0x7fc4) {
            rm->romMapper = value & 0x0f;
            slotMapPage(rm->slot, rm->sslot, rm->startPage, rm->romData + rm->romMapper * 0x2000, 1, 0);
		}
	}
}

static void rommappera1fm_reset(void *data)
{
    RomMapperA1FMModem *rm = (RomMapperA1FMModem*)data;
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->romData + rm->romMapper * 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);
}

int romMapperA1FMModemCreate(const char* filename, UInt8* romData, 
                             int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { rommappera1fm_destroy, rommappera1fm_reset, rommappera1fm_saveState, rommappera1fm_loadState };
    RomMapperA1FMModem *rm    = (RomMapperA1FMModem*)malloc(sizeof(RomMapperA1FMModem));

    rm->deviceHandle = deviceManagerRegister(ROM_FSA1FMMODEM, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 2, rommappera1fm_read, rommappera1fm_read, rommappera1fm_write, rommappera1fm_destroy, rm);

    rm->romData = (UInt8*)malloc(size);
    memcpy(rm->romData, romData, size);
    rm->romSize = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper = 0;

    rommappera1fm_reset(rm);

    return 1;
}

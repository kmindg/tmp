/***************************************************************************
 *  terminator_drive_sata_plugin_main.c
 ***************************************************************************
 *
 *  Description
 *      APIs to emulate the sata protocol
 *
 *
 *  History:
 *      11/11/08    guov    Created
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_sata_interface.h"
#include "fbe_terminator_file_api.h"
#include "fbe_terminator_common.h"
#include "terminator_drive.h"

/* this defines the interface for the plugin to call back to Terminator to get the data it needs */
#include "fbe_terminator.h"

#include "EmcPAL_Memory.h"
/**********************************/
/*        local variables         */
/**********************************/
/*  This is the old template - a new one is used below
 *  to match the inscription data also below
 *  TODO delete this at some point */
static fbe_u16_t sata_identify_device_template []  = {
 0x045a, 0x3fff, 0xc837, 0x0010, 0x0000, 0x0000, 0x003f, 0x0000,
 0x0000, 0x0000, 0x5042, 0x4741, 0x4a41, 0x5946, 0x2020, 0x2020,
 0x2020, 0x2020, 0x2020, 0x2020, 0x0003, 0xf36a, 0x0034, 0x474b,
 0x414f, 0x4139, 0x3441, 0x4855, 0x4137, 0x3231, 0x3031, 0x304b,
 0x4c41, 0x3333, 0x3020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020,
 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x8010,
 0x0000, 0x2f00, 0x4000, 0x0200, 0x0200, 0x0007, 0x3fff, 0x0010,
 0x003f, 0xfc10, 0x00fb, 0x0100, 0xffff, 0x0fff, 0x0000, 0x0407,
 0x0003, 0x0078, 0x0078, 0x0078, 0x0078, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x000f, 0x1706, /*0x17xx bit 1 3Gb/s bit 2 1.5Gb/s*/ 0x0000, 0x005e, 0x0040,
 0x00fc, 0x001a, 0x346b, 0x7fe9, 0x4123, 0x3449, 0x3e01, 0x4023,
 0x007f, 0x00aa, 0x0000, 0x0000, 0xfffe, 0x0000, 0x80fe, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0xfeef, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x5a87, 0x5000, 0xcca2, 0x1ec4, 0xc919,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4014,
 0x4014, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0001, 0x000e, 0x0000, 0x0000, 0x288a, 0x0cb1, 0xfe20, 0x0001,
 0x4000, 0x0400, 0x0000, 0x0000, 0x0000, 0x5dbd, 0xad67, 0x4603,
 0x0800, 0x0280, 0x3f7f, 0x00c0, 0x0040, 0xb000, 0x8000, 0x0000,
 0x4156, 0x4339, 0x0000, 0x686a, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x003f, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xcaa5
};

/* This is the new template - enable it at some point
static fbe_u16_t sata_identify_device_template []  = {
0x0C5A, 0x3FFF, 0xC837, 0x0010, 0x0000, 0x0000, 0x003F, 0x0000,
0x0000, 0x0000, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020,
0x3351, 0x4431, 0x3539, 0x3454, 0x0000, 0x8000, 0x0004, 0x4241,
0x4B36, 0x4C39, 0x2020, 0x5354, 0x3337, 0x3530, 0x3634, 0x304E,
0x5320, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020,
0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x2020, 0x8010,
0x0000, 0x2F00, 0x4000, 0x0200, 0x0200, 0x0007, 0x3FFF, 0x0010,
0x003F, 0xFC10, 0x00FB, 0x0010, 0xFFFF, 0x0FFF, 0x0000, 0x0007,
0x0003, 0x0078, 0x0078, 0x00F0, 0x0078, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x001F, 0x0506, 0x0000, 0x0048, 0x0040,
0x00FE, 0x0000, 0x346B, 0x7D01, 0x5923, 0x3449, 0x3C01, 0x4023,
0x407F, 0x0000, 0x0000, 0xFEFE, 0xFFFE, 0x0000, 0xD000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x66F0, 0x5754, 0x0000, 0x0000,
0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000, 0x0002,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0001, 0x66F0, 0x5754, 0x66F0, 0x5754, 0x2020, 0x0002, 0x02B4,
0x0002, 0x008A, 0x3C06, 0x3C0A, 0x0000, 0x07C6, 0x0100, 0x0800,
0x1314, 0x3000, 0x0002, 0x0080, 0x0000, 0x0000, 0x00A0, 0x0202,
0x0000, 0x0404, 0x0000, 0x0000, 0x0000, 0x0000, 0x1200, 0x000B,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x003D, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4AA5
};
*/


static fbe_u8_t sata_inscription_data_template []  = {
0x45, 0x4D, 0x43, 0x32, 0x53, 0x41, 0x54, 0x41,
0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x1C, 0x20,
0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
0x00, 0x00, 0x01, 0x0E, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x04, 0x00, 0x7C, 0x00, 0x00, 0x02, 0x41,
0x54, 0x41, 0x2D, 0x53, 0x54, 0x20, 0x20, 0x33,
0x37, 0x35, 0x30, 0x36, 0x34, 0x30, 0x4E, 0x20,
0x43, 0x4C, 0x41, 0x52, 0x31, 0x00, 0x00, 0x4B,
0x36, 0x4C, 0x39, 0x33, 0x51, 0x44, 0x31, 0x35,
0x39, 0x34, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x44,
0x47, 0x31, 0x31, 0x38, 0x30, 0x33, 0x32, 0x35,
0x35, 0x31, 0x00, 0x00, 0x30, 0x30, 0x35, 0x30,
0x34, 0x38, 0x37, 0x37, 0x37, 0x00, 0x00, 0x00,
0x53, 0x54, 0x33, 0x37, 0x35, 0x30, 0x36, 0x34,
0x30, 0x4E, 0x53, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x42, 0x41, 0x4B, 0x36, 0x4C, 0x39, 0x20, 0x20,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static fbe_u8_t sata_smart_read_data_template []  = {
0x0A, 0x00, 0x01, 0x0F, 0x00, 0x6F, 0x5A, 0x20,
0x5E, 0x07, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03,
0x00, 0x5A, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x04, 0x32, 0x00, 0x54, 0x54, 0x13,
0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x33,
0x00, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x07, 0x0F, 0x00, 0x57, 0x3C, 0x8A,
0x54, 0x7D, 0x25, 0x00, 0x00, 0x00, 0x09, 0x32,
0x00, 0x5E, 0x5E, 0xBF, 0x17, 0x00, 0x00, 0x00,
0x00, 0x00, 0x0A, 0x13, 0x00, 0x64, 0x63, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x32,
0x00, 0x54, 0x54, 0x16, 0x43, 0x00, 0x00, 0x00,
0x00, 0x00, 0xBB, 0x32, 0x00, 0x62, 0x62, 0x02,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBD, 0x3A,
0x00, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xBE, 0x22, 0x00, 0x47, 0x35, 0x1D,
0x00, 0x1C, 0x1D, 0x00, 0x00, 0x00, 0xC2, 0x22,
0x00, 0x1D, 0x2F, 0x1D, 0x00, 0x00, 0x00, 0x17,
0x00, 0x00, 0xC3, 0x1A, 0x00, 0x64, 0x3A, 0x23,
0xD3, 0x03, 0x06, 0x00, 0x00, 0x00, 0xC5, 0x12,
0x00, 0x63, 0x63, 0x16, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xC6, 0x10, 0x00, 0x63, 0x63, 0x16,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x3E,
0x00, 0xC8, 0xC8, 0x02, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xC8, 0x00, 0x00, 0x64, 0xFD, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0x32,
0x00, 0x64, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x82, 0x00, 0xAE, 0x01, 0x00, 0x5B,
0x03, 0x00, 0x01, 0x00, 0x01, 0xCA, 0x02, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0A,
0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x01,
0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
0x2E, 0x5B, 0x68, 0x52, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x3D, 0xE3, 0xB6, 0x9C, 0x99, 0x02,
0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x2E, 0x5B, 0x68, 0x52,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
0x00, 0x00, 0x5D, 0x43, 0x70, 0xC1, 0x02, 0x00,
0x00, 0x00, 0xDB, 0x04, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xBE, 0x17, 0x00, 0x00, 0x00, 0x00,
0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF4
};

static fbe_u8_t sata_read_log_ext_page_10_data_template []  = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*
static fbe_u8_t sata_read_native_max_address_ext_abort_template [] = {
     0x34,0x40,0x51,0x04,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};
*/

/*
static fbe_u8_t sata_read_native_max_address_ext_template [] = {
     0x34,0x40,0x50,0x00,0xaf,0x6d,0x70,0x44,
     0x74,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};
*/

static fbe_u8_t sata_read_native_max_address_ext_template [] = {
                    0x34,0x40,0x50,0x00,0xF0,0xFE,0x00,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static fbe_u8_t sata_flush_cache_ext_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

static fbe_u8_t sata_set_features_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

static fbe_u8_t sata_read_sectors_ext_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/*
static fbe_u8_t sata_smart_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};
*/

/* This response FIS indicates that the GList HAS NOT exceeded
   threshold */
static fbe_u8_t sata_smart_return_status_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x4F,0xC2,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/* This response FIS indicates that the GList HAS exceeded
   threshold */
/*
static fbe_u8_t sata_smart_return_status_threshold_exceeded_template [] = {
     0x34,0x40,0x50,0x00,0x00,0xF4,0x2C,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};
*/

/* SMART Write Log Response FIS to set the Write Error Recovery Timer */
static fbe_u8_t sata_smart_write_log_error_recovery_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/* Response FIS for Read Log Ext command for Log Page 10 */
static fbe_u8_t sata_read_log_ext_log_page_10_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/* Response FIS for Execute Device Diagnostic */
static fbe_u8_t sata_execute_device_diagnostic_template [] = {
     0x34,0x40,0x50,0x01,0x01,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/* Response FIS for Check Power Mode - the response FIS below
 * with 0xFF in the Count register specifies "Device is in PM0:
 * Active state or PM1: Idle State".
 */
static fbe_u8_t sata_check_power_mode_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/* Response FIS for Download Microcode */
static fbe_u8_t sata_download_microcode_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/* Response FIS for standby*/
static fbe_u8_t sata_standby_template [] = {
     0x34,0x40,0x50,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00
};

/******************************/
/*     local function         */
/*****************************/
static fbe_status_t terminator_drive_fis_protocol_entry(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_identify_device(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_read_native_max_address_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_flush_cache_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_set_features(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_read_sectors_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_smart(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_read_log_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_check_power_mode(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_execute_device_diagnostic(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_download_microcode(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_read_fpdma_queued(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_write_fpdma_queued(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle);
static fbe_status_t terminator_drive_fis_standby(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle);

static fbe_status_t terminator_drive_fis_process_xfer(fbe_terminator_io_t *terminator_io,
                                                    fbe_terminator_device_ptr_t drive_handle,
                                                    sas_drive_xfer_data_t *xfer_data);

static fbe_status_t terminator_drive_fis_prepare_xfer(fbe_payload_ex_t * payload,
                                                        fbe_terminator_device_ptr_t drive_handle,
                                                        sas_drive_xfer_data_t *xfer_data);


static fbe_status_t terminator_drive_fis_prepare_read_fpdma_queued(fbe_payload_ex_t * payload,
                                                                    fbe_terminator_device_ptr_t drive_handle,
                                                                    sas_drive_xfer_data_t *xfer_data);

static fbe_status_t terminator_drive_fis_prepare_write_fpdma_queued(fbe_payload_ex_t * payload,
                                                                    fbe_terminator_device_ptr_t drive_handle,
                                                                    sas_drive_xfer_data_t *xfer_data);

/*********************************************************************
 *              fbe_terminator_sata_drive_payload ()
 *********************************************************************
 *
 *  Description: this function understands the payload and extracts fis_operation
 *  out of payload
 *
 *  Inputs: payload and drive handle
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/18/08: guov    created
 *
 *********************************************************************/
fbe_status_t fbe_terminator_sata_drive_payload(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_ex_t *payload = terminator_io->payload;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    if (fis_operation == NULL) {
        /* log the error */
        return status;
    }
    status = terminator_drive_fis_protocol_entry(terminator_io, drive_handle);
    return status;
}

/*********************************************************************
 *              terminator_drive_fis_protocol_entry ()
 *********************************************************************
 *
 *  Description: emulating FIS response
 *
 *  Inputs: FIS and drive handle
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/11/08: guov    created
 *
 *********************************************************************/
static fbe_status_t
terminator_drive_fis_protocol_entry(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_ex_t *payload = terminator_io->payload;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    switch(fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET]) {
    case FBE_SATA_PIO_COMMAND_IDENTIFY_DEVICE:
        terminator_drive_fis_identify_device(payload, drive_handle);
        break;
    case FBE_SATA_PIO_COMMAND_READ_NATIVE_MAX_ADDRESS_EXT:
        terminator_drive_fis_read_native_max_address_ext(payload, drive_handle);
        break;
    case FBE_SATA_PIO_COMMAND_FLUSH_CACHE_EXT:
                terminator_drive_fis_flush_cache_ext(payload, drive_handle);
                break;
       /* Set Features supports Disable Write Cache, Enable RLA, Set PIO Mode
        * and Set UDMA Mode  */
    case FBE_SATA_PIO_COMMAND_SET_FEATURES:
                terminator_drive_fis_set_features(payload, drive_handle);
                break;
        case FBE_SATA_PIO_COMMAND_READ_SECTORS_EXT:
                terminator_drive_fis_read_sectors_ext(payload, drive_handle);
                break;
       /* SMART supports Enable Operations, Set Autosave, Return Status (for
        * GList information), Read Data (for various error counts)and the SCT
        * command set.  The SCT command supports setting the read and write
        * error recovery timers. */
        case FBE_SATA_PIO_COMMAND_SMART:
                terminator_drive_fis_smart(payload, drive_handle);
                break;
        case FBE_SATA_PIO_COMMAND_READ_LOG_EXT:
                terminator_drive_fis_read_log_ext(payload, drive_handle);
                break;
    case FBE_SATA_PIO_COMMAND_EXECUTE_DEVICE_DIAGNOSTIC:
        terminator_drive_fis_execute_device_diagnostic(payload, drive_handle);
        break;
    case FBE_SATA_PIO_COMMAND_CHECK_POWER_MODE:
        terminator_drive_fis_check_power_mode(payload, drive_handle);
        break;
    case FBE_SATA_READ_FPDMA_QUEUED:
        terminator_drive_fis_read_fpdma_queued(terminator_io, drive_handle);
        break;
    case FBE_SATA_WRITE_FPDMA_QUEUED:
        terminator_drive_fis_write_fpdma_queued(terminator_io, drive_handle);
        break;
    case FBE_SATA_PIO_COMMAND_DOWNLOAD_MICROCODE:
        terminator_drive_fis_download_microcode(payload, drive_handle);
        break;
    
    case FBE_SATA_PIO_COMMAND_STANDBY_IMMEDIATE:
        terminator_drive_fis_standby(payload, drive_handle);
        break;

    default:
        break;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_identify_device(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;
    fbe_sg_element_t * sg_list = NULL;
    fbe_lba_t block_count;
	terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
	fbe_u8_t * identify_data = NULL;
    fbe_terminator_sata_drive_info_t drive_info;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    fbe_copy_memory(sg_list[0].address, sata_identify_device_template, transfer_count);

	identify_data = sg_list[0].address;
	block_count = drive->capacity;

	identify_data[205] = (fbe_u8_t) (block_count >> 40);
	identify_data[204] = (fbe_u8_t) (block_count >> 32);
	identify_data[203] = (fbe_u8_t) (block_count >> 24);
	identify_data[202] = (fbe_u8_t) (block_count >> 16);
	identify_data[201] = (fbe_u8_t) (block_count >> 8);
	identify_data[200] = (fbe_u8_t) block_count ;

    /* get serial number of this drive and copy it into the return buffer */
    terminator_get_sata_drive_info(drive_handle, &drive_info);
    fbe_copy_memory(sg_list[0].address+FBE_SATA_SERIAL_NUMBER_OFFSET,
                    drive_info.drive_serial_number,
                    FBE_SATA_SERIAL_NUMBER_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_read_native_max_address_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;
	terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
	fbe_lba_t block_count;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    fbe_copy_memory(fis_operation->response_buffer, sata_read_native_max_address_ext_template, FBE_SATA_READ_NATIVE_MAX_ADDRESS_EXT_DATA_SIZE);

	block_count = drive->capacity;
	fis_operation->response_buffer[10] = (fbe_u8_t) (block_count >> 40);
	fis_operation->response_buffer[9] = (fbe_u8_t) (block_count >> 32);
	fis_operation->response_buffer[8] = (fbe_u8_t) (block_count >> 24);
	fis_operation->response_buffer[6] = (fbe_u8_t) (block_count >> 16);
	fis_operation->response_buffer[5] = (fbe_u8_t) (block_count >> 8);
	fis_operation->response_buffer[4] = (fbe_u8_t) drive->capacity ;

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_flush_cache_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    fbe_copy_memory(fis_operation->response_buffer,
                         sata_flush_cache_ext_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_set_features(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    switch(fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET]) {

        case FBE_SATA_SET_FEATURES_DISABLE_WCACHE:
            /* TODO update Identify template*/
            break;

        case FBE_SATA_SET_FEATURES_ENABLE_RLA:
            /* TODO update Identify template*/
            break;

        case FBE_SATA_SET_FEATURES_TRANSFER:
            switch (fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET]) {
            case FBE_SATA_TRANSFER_PIO_SLOW:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_PIO_0:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_PIO_1:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_PIO_2:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_PIO_3:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_PIO_4:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_0:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_1:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_2:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_3:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_4:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_5:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_6:
                /* TODO update Identify template */
                break;
            case FBE_SATA_TRANSFER_UDMA_7:
                /* TODO update Identify template */
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }

        /* The response FIS for all of the above will be a normal output
         * error free response FIS
         */
    fbe_copy_memory(fis_operation->response_buffer,
                         sata_set_features_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_read_sectors_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;
    fbe_sg_element_t * sg_list = NULL;
    fbe_lba_t fis_lba = 0;
	terminator_drive_t * drive = (terminator_drive_t *)drive_handle;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    fbe_payload_fis_get_lba(fis_operation, &fis_lba);

        if ( (fis_lba == drive->capacity - 1) && (transfer_count == FBE_SATA_DISK_DESIGNATOR_BLOCK_SIZE) )
        {
            /* Return the inscription data from the template */
            fbe_copy_memory(sg_list[0].address, sata_inscription_data_template, transfer_count);
        }

        /* Fill in the response FIS */
    fbe_copy_memory(fis_operation->response_buffer,
                         sata_read_sectors_ext_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;

}

static fbe_status_t
terminator_drive_fis_smart(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_sg_element_t * sg_list = NULL;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

        if (fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET] ==
                                                          FBE_SATA_SMART_LBA_MID &&
            fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_HIGH_OFFSET] ==
                                                          FBE_SATA_SMART_LBA_HI)
        {
            switch(fis_operation->fis[FBE_SATA_COMMAND_FIS_FEATURES_OFFSET]) {

            case FBE_SATA_SMART_ENABLE_OPERATIONS:
                /* TODO update Identify template*/
                fbe_copy_memory(fis_operation->response_buffer,
                                 sata_smart_write_log_error_recovery_template,
                                 FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);
                break;

            case FBE_SATA_SMART_ATTRIB_AUTOSAVE:
                if (fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] ==
                                             FBE_SATA_SMART_ATTRIB_AUTOSAVE_ENABLE)
                {
                    /* Enable SMART attribute Autosave */
                    /* TODO update Identify template*/
                }
                else if (fis_operation->fis[FBE_SATA_COMMAND_FIS_COUNT_OFFSET] ==
                                             FBE_SATA_SMART_ATTRIB_AUTOSAVE_DISABLE)
                {
                      /* Disable SMART attribute Autosave */
                      /* TODO update Identify template*/
                }
                else
                {
                      /* Vendor specific count register setting */
                }
                fbe_copy_memory(fis_operation->response_buffer,
                                     sata_smart_write_log_error_recovery_template,
                                     FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);
                break;

            case FBE_SATA_SMART_RETURN_STATUS:
                /* Return the response FIS that has the status information
                 * in the LBA Hi and LBA mid registers. */
                fbe_copy_memory(fis_operation->response_buffer,
                                 sata_smart_return_status_template,
                                 FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);
                break;

            case FBE_SATA_SMART_READ_DATA_SUBCMD:
                fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

                /* Return the data block template for the SMART read data command
                 * Note: The transfer count is usualy specified (as 1 block) in the command
                 *       FIS but the ATA8 spec. shows the Count register as a n/a. So
                 *       rather than get it out of the command FIS we use
                 *       FBE_SATA_SMART_SECTOR_COUNT
                 */
                 fbe_copy_memory(sg_list[0].address, sata_smart_read_data_template, FBE_SATA_SMART_SECTOR_COUNT);
                 break;

            case FBE_SATA_SMART_WRITE_LOG_SUBCMD:
                fbe_copy_memory(fis_operation->response_buffer,
                                 sata_smart_write_log_error_recovery_template,
                                 FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);
                break;

            default:
                break;

            }

        }

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_read_log_ext(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;
    fbe_sg_element_t * sg_list = NULL;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);
    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    /* Check to see if the request is for log page 0x10 and that
     *     the first log page (the only page) of log 0x10 is being requested.
     *     and that the block count is for 1 block
     */
        if ( (fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_LOW_OFFSET] == 0x10) &&
             (fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_OFFSET] == 0x00) &&
             (fis_operation->fis[FBE_SATA_COMMAND_FIS_LBA_MID_EXT_OFFSET] == 0x00) &&
             (transfer_count == 1) )
        {
            /* Return the data for log page 0x10 from the template */
            fbe_copy_memory(sg_list[0].address, sata_read_log_ext_page_10_data_template, transfer_count);
        }

        /* Fill in the response FIS */
    fbe_copy_memory(fis_operation->response_buffer,
                         sata_read_log_ext_log_page_10_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;

}

static fbe_status_t
terminator_drive_fis_execute_device_diagnostic(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_copy_memory(fis_operation->response_buffer,
                         sata_execute_device_diagnostic_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_check_power_mode(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_copy_memory(fis_operation->response_buffer,
                         sata_check_power_mode_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_download_microcode(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t *   fis_operation = NULL;

    /*we just do a small delay to simulate the actual download time*/
    fbe_thread_delay(4000);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_copy_memory(fis_operation->response_buffer,
                         sata_download_microcode_template,
                         FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_standby(fbe_payload_ex_t * payload, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_payload_fis_operation_t *   fis_operation = NULL;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

	fbe_copy_memory(fis_operation->response_buffer,
	                     sata_standby_template,
	                     FBE_SATA_REGISTER_DEVICE_TO_HOST_FIS_SIZE);

    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_read_fpdma_queued(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t transfer_count = 0;
    sas_drive_xfer_data_t xfer_data;
    fbe_payload_ex_t *payload = terminator_io->payload;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    /* fbe_copy_memory(fis_operation->response_buffer, sata_read_native_max_address_ext_template, FBE_SATA_READ_NATIVE_MAX_ADDRESS_EXT_DATA_SIZE); */

    status = terminator_drive_fis_prepare_read_fpdma_queued(payload, drive_handle, &xfer_data);
    if (status != FBE_STATUS_OK) { return status; }
    status = terminator_drive_fis_prepare_xfer(payload, drive_handle, &xfer_data);
    if (status != FBE_STATUS_OK) { return status; }


    status = terminator_drive_fis_process_xfer(terminator_io, drive_handle, &xfer_data);

    fbe_payload_fis_set_transferred_count(fis_operation, transfer_count);    

    return status;
}

static fbe_status_t
terminator_drive_fis_write_fpdma_queued(fbe_terminator_io_t *terminator_io, fbe_terminator_device_ptr_t drive_handle)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_payload_ex_t * payload = terminator_io->payload;
    fbe_u32_t transfer_count;
    sas_drive_xfer_data_t xfer_data;
    fbe_u8_t tag;

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    fbe_payload_fis_get_tag(fis_operation, &tag);
    /* terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Tag = %X\n", __FUNCTION__,tag); */

    /* fbe_copy_memory(fis_operation->response_buffer, sata_read_native_max_address_ext_template, FBE_SATA_READ_NATIVE_MAX_ADDRESS_EXT_DATA_SIZE); */

    status = terminator_drive_fis_prepare_write_fpdma_queued(payload, drive_handle, &xfer_data);
    if (status != FBE_STATUS_OK) { return status; }
    status = terminator_drive_fis_prepare_xfer(payload, drive_handle, &xfer_data);
    if (status != FBE_STATUS_OK) { return status; }
    status = terminator_drive_fis_process_xfer(terminator_io, drive_handle,&xfer_data);

    return status;
}

static fbe_status_t
terminator_drive_fis_prepare_read_fpdma_queued(fbe_payload_ex_t * payload,
    fbe_terminator_device_ptr_t drive_handle,
    sas_drive_xfer_data_t *xfer_data)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_status_t status;

    /* Get fis operation */
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    /* Calculate the lba and blocks. */
    fbe_payload_fis_get_lba(fis_operation, &xfer_data->lba);
    fbe_payload_fis_get_block_count(fis_operation, &xfer_data->blocks);

    /* set the direction */
    xfer_data->direction_in = TRUE;
    /* fill in the common data */
    status = terminator_drive_fis_prepare_xfer(payload, drive_handle, xfer_data);
    return status;
}

static fbe_status_t
terminator_drive_fis_prepare_write_fpdma_queued(fbe_payload_ex_t * payload,
    fbe_terminator_device_ptr_t drive_handle,
    sas_drive_xfer_data_t *xfer_data)
{
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_status_t status;

    /* Get fis operation */
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    /* Calculate the lba and blocks. */
    fbe_payload_fis_get_lba(fis_operation, &xfer_data->lba);
    fbe_payload_fis_get_block_count(fis_operation, &xfer_data->blocks);

    /* set the direction */
    xfer_data->direction_in = FALSE;
    /* fill in the common data */
    status = terminator_drive_fis_prepare_xfer(payload, drive_handle, xfer_data);
    return status;
}

static fbe_status_t
terminator_drive_fis_prepare_xfer(fbe_payload_ex_t * payload,
    fbe_terminator_device_ptr_t drive_handle,
    sas_drive_xfer_data_t *xfer_data)
{
    fbe_status_t                        status;
    fbe_u32_t                           port_number;
    fbe_u32_t                           enclosure_number;
    fbe_u32_t                           slot_number;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_sas_drive_type_t drive_type;
    fbe_block_size_t block_size;
    int count = 0;
    fbe_u32_t            transfer_count = 0;

    status = terminator_get_port_index(drive_handle, &port_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get port number thru drive handle\n", __FUNCTION__);
        /* fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST); */
        return status;
    }
    status = terminator_get_drive_enclosure_number(drive_handle, &enclosure_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get enclosure_number thru drive handle\n", __FUNCTION__);
        //fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return status;
    }
    status = terminator_get_drive_slot_number(drive_handle, &slot_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get slot_number thru drive handle\n", __FUNCTION__);
        //fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return status;
    }
    count = _snprintf(xfer_data->drive_name, sizeof(xfer_data->drive_name), TERMINATOR_DISK_FILE_NAME_FORMAT, port_number, enclosure_number, slot_number);
    if (( count < 0 ) || (sizeof(xfer_data->drive_name) == count )) {
         
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    fbe_payload_fis_get_transfer_count(fis_operation, &transfer_count);

    /* Validate request doesn't exceed per-drive transfer size*/
    if (transfer_count > drive_get_maximum_transfer_bytes(drive_handle))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s transfer_count: 0x%x is greater than maximum: 0x%x\n", 
                         __FUNCTION__, transfer_count, drive_get_maximum_transfer_bytes(drive_handle));
        fbe_payload_fis_set_request_status(fis_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST); 
        /* It's ok to return an error since the terminator ignores the return status */
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get fis operation */
    fis_operation = fbe_payload_ex_get_fis_operation(payload);

    terminator_drive_get_type( drive_handle, &drive_type);

    block_size = 512; /* Temporary hack */

    xfer_data->block_size = block_size; /*payload_cdb_operation->block_size*/
    /* FIXME!!! what's the sg_p in payload???*/
    //xfer_data->sg_desc_p = &payload_cdb_operation->payload_sg_descriptor;

    /* Set pointer to sg descriptor */
    fbe_payload_fis_get_sg_descriptor(fis_operation, &xfer_data->payload_sg_desc_p);

    fbe_payload_ex_get_sg_list(payload, &xfer_data->sg_p, NULL);
    fbe_payload_ex_get_pre_sg_list(payload, &xfer_data->pre_sg_p);
    fbe_payload_ex_get_post_sg_list(payload, &xfer_data->post_sg_p);

    /* fbe_payload_fis_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SUCCESS); */
    return FBE_STATUS_OK;
}

static fbe_status_t
terminator_drive_fis_process_xfer(fbe_terminator_io_t *terminator_io,
                       fbe_terminator_device_ptr_t drive_handle,
                       sas_drive_xfer_data_t *xfer_data)
{
    /* By default we return good status. */
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u32_t repeat_count;
    fbe_u64_t device_byte;
    fbe_u32_t bytes_left_to_xfer;
    fbe_sg_element_t *current_sg_p;
    fbe_payload_sg_index_t sg_list_index;
    terminator_drive_t * drive = (terminator_drive_t *)drive_handle;
    fbe_u8_t * data_buffer = NULL;
	fbe_u8_t * data_buffer_pointer = NULL;
    fbe_u32_t sg_bytes_to_xfer;
    fbe_u32_t bytes_received;
    char file_name[FBE_MAX_PATH + MAX_CONFIG_FILE_NAME];
    fbe_payload_ex_t *payload = terminator_io->payload;

    EmcpalZeroVariable(file_name);
    fis_operation = fbe_payload_ex_get_fis_operation(payload);
    if(fis_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s fbe_payload_ex_get_cdb_operation failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_sg_descriptor_get_repeat_count(xfer_data->payload_sg_desc_p, &repeat_count);

    if(repeat_count == 0){ /* Not initialized */
        repeat_count = 1;
    }

    terminator_io->opcode = (xfer_data->direction_in) ? FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ : FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
    terminator_io->lba = xfer_data->lba;
    terminator_io->block_count = xfer_data->blocks;
    terminator_io->block_size = xfer_data->block_size;
    terminator_io->device_ptr = drive;

    device_byte = xfer_data->lba * xfer_data->block_size;
    /* bytes_left_to_xfer = (fbe_u32_t) (xfer_data->blocks * xfer_data->block_size); */
    sg_list_index = FBE_PAYLOAD_SG_INDEX_INVALID;

    /* Check if xfer has valid LBA */
    if (xfer_data->lba + xfer_data->blocks > drive->capacity)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s drive %s attempt to seek past end of disk (0x%llx).\n", __FUNCTION__, xfer_data->drive_name, (unsigned long long)device_byte);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   {
 //       KvTrace("%s drive %s attempt to seek past end of disk (0x%llx).\n",
//                      __FUNCTION__, xfer_data->drive_name, device_byte);
            /* We have three sg lists.  We start with whichever is not empty.
            * If they are all empty the we return error.
            */
        if (xfer_data->pre_sg_p != NULL && fbe_sg_element_count(xfer_data->pre_sg_p) != 0) {
            sg_list_index = FBE_PAYLOAD_SG_INDEX_PRE_SG_LIST;
            current_sg_p = xfer_data->pre_sg_p;
        } else if (xfer_data->sg_p != NULL && fbe_sg_element_count(xfer_data->sg_p) != 0) {
            sg_list_index = FBE_PAYLOAD_SG_INDEX_SG_LIST;
            current_sg_p = xfer_data->sg_p;
        } else if (xfer_data->post_sg_p != NULL && fbe_sg_element_count(xfer_data->post_sg_p) != 0) {
            sg_list_index = FBE_PAYLOAD_SG_INDEX_POST_SG_LIST;
            current_sg_p = xfer_data->post_sg_p;
        }
        else {
            /* None of the sg lists is any good. */
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s drive %s end of SG list hit unexpectedly.\n", __FUNCTION__, xfer_data->drive_name);
                /* terminator_drive_increment_error_count(drive_handle);*/
            /* Set status to a value which the physical drive will not handle so it will not
                * do retries.  We want an error to bubble up immediately.
                */
            /* fbe_payload_cdb_set_request_status(payload_cdb_operation_p, FBE_PORT_REQUEST_STATUS_PENDING); */
            return FBE_STATUS_GENERIC_FAILURE;
        }

        bytes_left_to_xfer = (fbe_u32_t) (xfer_data->blocks * xfer_data->block_size);
        data_buffer = (fbe_u8_t *)fbe_terminator_allocate_memory(bytes_left_to_xfer);
        if (data_buffer == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for data buffer at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        data_buffer_pointer = data_buffer;

        if (xfer_data->direction_in) {
            /* Read the data */
            terminator_drive_trace_read(drive, terminator_io, FBE_TRUE /* b_start */);
            terminator_drive_read(drive, xfer_data->lba, (bytes_left_to_xfer / xfer_data->block_size), xfer_data->block_size, data_buffer, terminator_io);	
        }

        while ( bytes_left_to_xfer > 0 )
        {
            /* If we hit the end of the sg list or we find a NULL address, then exit with error. */
            if (current_sg_p == NULL || fbe_sg_element_address(current_sg_p) == NULL || fbe_sg_element_count(current_sg_p) == 0 ) {
                /* Error, we could not transfer what we expected. */
                bytes_received = 0;
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s drive %s end of SG list hit unexpectedly.\n", __FUNCTION__, xfer_data->drive_name);
                /* terminator_drive_increment_error_count(drive_handle); */
                /* Set status to a value which the physical drive will not handle so it will not
                    * do retries.  We want an error to bubble up immediately.
                    */
                /* fbe_payload_cdb_set_request_status(payload_cdb_operation_p, FBE_PORT_REQUEST_STATUS_PENDING); */
                fbe_terminator_free_memory(data_buffer);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            sg_bytes_to_xfer = fbe_sg_element_count(current_sg_p);

            if (sg_bytes_to_xfer > bytes_left_to_xfer) {
                sg_bytes_to_xfer = bytes_left_to_xfer; /* The sg is longer than this transfer, simply transfer the amount specified. */
            }

            /* Transfer bytes. */
            if (xfer_data->direction_in) {
                /* fbe_copy_memory(void * dst, const void * src, fbe_u32_t length) */
                fbe_copy_memory(fbe_sg_element_address(current_sg_p), data_buffer_pointer, sg_bytes_to_xfer);
                data_buffer_pointer += sg_bytes_to_xfer;
                bytes_received = sg_bytes_to_xfer;
            } else {
                /* fbe_copy_memory(void * dst, const void * src, fbe_u32_t length) */
                fbe_copy_memory(data_buffer_pointer, fbe_sg_element_address(current_sg_p), sg_bytes_to_xfer);
                data_buffer_pointer += sg_bytes_to_xfer;
                bytes_received = sg_bytes_to_xfer;
            }

            if (bytes_received != sg_bytes_to_xfer) { /* Check if we transferred enough. */
                /* Error, we didn't transfer what we expected. */
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s drive %s unexpected transfer.\n", __FUNCTION__, xfer_data->drive_name);
                /* terminator_drive_increment_error_count(drive_handle); */
                /* Set status to a value which the physical drive will not handle so it will not
                    * do retries.  We want an error to bubble up immediately. */
                /* fbe_payload_cdb_set_request_status(payload_cdb_operation_p, FBE_PORT_REQUEST_STATUS_PENDING); */
                fbe_terminator_free_memory(data_buffer);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (repeat_count == 1)
            {
                current_sg_p++;
            }
            bytes_left_to_xfer -= sg_bytes_to_xfer;

            /* When we hit the end of one SG list, try to goto the next list. Only goto the next list if we are not already at
             * the end. There are three sg list and we will iterate over each in turn.
             */
            if (bytes_left_to_xfer > 0 && fbe_sg_element_count(current_sg_p) == 0 &&  sg_list_index < FBE_PAYLOAD_SG_INDEX_POST_SG_LIST) {
                sg_list_index++; /* Increment to the next sg list. */
                if (sg_list_index == FBE_PAYLOAD_SG_INDEX_SG_LIST) { /* We now are either at the middle sg or the end sg. */
                    current_sg_p = xfer_data->sg_p; /* Get the middle sg list ptr. */
                } else if (sg_list_index == FBE_PAYLOAD_SG_INDEX_POST_SG_LIST) {
                    current_sg_p = xfer_data->post_sg_p; /* Get the end sg list ptr. */
                }
            } /* If we hit the end of an sg list. */

        } /* end while bytes left and no error */

        /* Perform actual write */
        if (!xfer_data->direction_in)
        {
            fbe_block_count_t write_blocks;
            write_blocks = (xfer_data->blocks * xfer_data->block_size) / xfer_data->block_size;
            /* Perform write */
            terminator_drive_trace_write(drive, terminator_io, FBE_TRUE /* b_start */);
            terminator_drive_write(drive, xfer_data->lba, write_blocks, xfer_data->block_size, data_buffer, terminator_io);			
        }

        fbe_terminator_free_memory(data_buffer);

    } /* repeat count loop */

    return status;
}

fbe_status_t fis_mark_payload_abort(fbe_payload_ex_t *payload)
{
    fbe_status_t status;
    fbe_payload_fis_operation_t * payload_fis_operation = NULL;

    payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
    /* make sure we have the right operation */
    if (payload_fis_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: Error ! Operation must be payload_fis_operation \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_payload_fis_set_request_status(payload_fis_operation, FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN);
    return status;
}

/* end fbe_terminator_sata_plugin_main.c */

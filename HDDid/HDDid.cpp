// HDDid.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <winioctl.h>

#define ID_CMD          0xEC            // Returns ID sector for ATA
#include <pshpack1.h>
typedef struct _GETVERSIONINPARAMS {
        BYTE     bVersion;               // Binary driver version.
        BYTE     bRevision;              // Binary driver revision.
        BYTE     bReserved;              // Not used.
        BYTE     bIDEDeviceMap;          // Bit map of IDE devices.
        DWORD   fCapabilities;          // Bit mask of driver capabilities.
        DWORD   dwReserved[4];          // For future use.
} GETVERSIONINPARAMS, *PGETVERSIONINPARAMS, *LPGETVERSIONINPARAMS;

typedef struct _DRIVERSTATUS {
        BYTE     bDriverError;           // Error code from driver,
                                                                // or 0 if no error.
        BYTE     bIDEError;                      // Contents of IDE Error register.
                                                                // Only valid when bDriverError
                                                                // is SMART_IDE_ERROR.
        BYTE     bReserved[2];           // Reserved for future expansion.
        DWORD   dwReserved[2];          // Reserved for future expansion.
} DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

typedef struct _IDEREGS {
        BYTE     bFeaturesReg;           // Used for specifying SMART "commands".
        BYTE     bSectorCountReg;        // IDE sector count register
        BYTE     bSectorNumberReg;       // IDE sector number register
        BYTE     bCylLowReg;             // IDE low order cylinder value
        BYTE     bCylHighReg;            // IDE high order cylinder value
        BYTE     bDriveHeadReg;          // IDE drive/head register
        BYTE     bCommandReg;            // Actual IDE command.
        BYTE     bReserved;                      // reserved for future use.  Must be zero.
} IDEREGS, *PIDEREGS, *LPIDEREGS;

typedef struct _SENDCMDINPARAMS {
        DWORD   cBufferSize;            // Buffer size in bytes
        IDEREGS irDriveRegs;            // Structure with drive register values.
        BYTE     bDriveNumber;           // Physical drive number to send
                                                                // command to (0,1,2,3).
        BYTE     bReserved[3];           // Reserved for future expansion.
        DWORD   dwReserved[4];          // For future use.
        BYTE     bBuffer[1];                     // Input buffer.
} SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;

typedef struct _SENDCMDOUTPARAMS {
        DWORD                   cBufferSize;            // Size of bBuffer in bytes
        DRIVERSTATUS            DriverStatus;           // Driver status structure.
        BYTE                    bBuffer[1];             // Buffer of arbitrary length in which to store the data read from the                                                                                  // drive.
} SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;

#include <poppack.h>

#define SMART_GET_VERSION               CTL_CODE(IOCTL_DISK_BASE, 0x0020, METHOD_BUFFERED, FILE_READ_ACCESS)
#define SMART_SEND_DRIVE_COMMAND        CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define SMART_RCV_DRIVE_DATA            CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IDENTIFY_BUFFER_SIZE        512

#pragma pack(1)

typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
    USHORT Reserved6[50];                   //     75-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()

CString GetSerialNumberOfAHardDisk()
{
	CString strHardDiskSerialNumber ;

	
	TCHAR szDriveName[256] ;
	HANDLE hDrive = 0;
	sprintf  (szDriveName, "\\\\.\\PhysicalDrive%d", 0) ;
	
	hDrive = CreateFile (szDriveName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDrive != INVALID_HANDLE_VALUE)
	{
		DWORD cbBytesReturned = 0 ;
		
		GETVERSIONINPARAMS objVersionParams ;
		memset (&objVersionParams, 0, sizeof(objVersionParams)) ;
		if (  DeviceIoControl (hDrive, SMART_GET_VERSION, NULL, 0, &objVersionParams, sizeof (GETVERSIONINPARAMS), &cbBytesReturned, NULL) )
		{         
			ULONG nCommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
			PSENDCMDINPARAMS pSendCommands = (PSENDCMDINPARAMS) malloc (nCommandSize) ;
			
			pSendCommands->irDriveRegs.bCommandReg = ID_CMD ;
			DWORD BytesReturned = 0 ;
			if (DeviceIoControl (hDrive, SMART_RCV_DRIVE_DATA, pSendCommands, sizeof(SENDCMDINPARAMS), pSendCommands, nCommandSize, &BytesReturned, NULL) )
			{
				WORD* pIdSector = (WORD *) (PIDENTIFY_DATA) ((PSENDCMDOUTPARAMS) pSendCommands)->bBuffer ;
				
				char szSerialNumber[100] = "" ;
				for (int index = 10, position=0; index <= 19; index++)
				{
					szSerialNumber[position] = (char) (pIdSector[index] / 256) ;
					position++ ;
					
					szSerialNumber[position] = (char) (pIdSector[index] % 256) ;
					position++ ;
				}

				strHardDiskSerialNumber = szSerialNumber ;
				strHardDiskSerialNumber.TrimLeft() ;
				strHardDiskSerialNumber.TrimRight() ;
			}
			
			CloseHandle (hDrive) ;
			free (pSendCommands) ;
			pSendCommands = NULL ;
		}
	}
	
	return strHardDiskSerialNumber ;
}

int _tmain(int argc, _TCHAR* argv[])
{
	GetSerialNumberOfAHardDisk();
	return 0;
}


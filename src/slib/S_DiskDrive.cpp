#include "S_OS.h"
#include "S_DiskDrive.h"

S_DiskDrive::S_DiskDrive(int iindex) {
   index = iindex;
   letter = (char)('A' + index);
   Update();
   }


int
S_DiskDrive::Update() {
   totalBytes = 0;
   availableBytes = 0;
#  ifdef OS2
      FSALLOCATE fsa;
      DosError (FERR_DISABLEHARDERR);
      if ( !DosQueryFSInfo(index+1,FSIL_ALLOC,&fsa,sizeof(FSALLOCATE)) ) {
         totalBytes = fsa.cSectorUnit*fsa.cUnit*fsa.cbSector;
         totalKBytes = (fsa.cSectorUnit*fsa.cUnit)/1024*fsa.cbSector;
         availableBytes = fsa.cSectorUnit*fsa.cUnitAvail*fsa.cbSector;
         availableKBytes = (fsa.cSectorUnit*fsa.cUnitAvail)/1024*fsa.cbSector;
         isAccessible = TRUE;
         }
      else {
         totalBytes = availableBytes = 0;
         isAccessible = FALSE;
         }
      DosError (FERR_ENABLEHARDERR);
#  endif
#  ifdef WIN32
      char driveRoot[4];
      sprintf(driveRoot,"%c:\\",letter);

      typedef BOOL (__stdcall *FNGetDiskFreeSpaceEx)(LPCTSTR,
						     PULARGE_INTEGER,
						     PULARGE_INTEGER,
						     PULARGE_INTEGER);
      FNGetDiskFreeSpaceEx pfn;
      pfn = (FNGetDiskFreeSpaceEx)
	GetProcAddress(GetModuleHandle("kernel32.dll"),
		       "GetDiskFreeSpaceExA");
      if ( pfn )
	{
          ULARGE_INTEGER available;
          ULARGE_INTEGER total;
          ULARGE_INTEGER free;

          isAccessible = pfn(driveRoot,
			     &available,
			     &total,
			     &free);
	  if ( isAccessible )
	    {

	      totalKBytes = (ulong)(*((_int64 *)&total)>>10);
              if ( total.HighPart != 0 )
		{
		  totalBytes = (ulong)0xFFFFFFFF;
		}
	      else
		{
                  totalBytes = total.LowPart;
		}

	      availableKBytes = (ulong)(*((_int64 *)&available)>>10);
              if ( available.HighPart != 0 )
		{
		  availableBytes = (ulong)0xFFFFFFFF;
		}
	      else
		{
                  availableBytes = total.LowPart;
		}

	    }

	}

      if ( !pfn )
	{
	  ulong sectorsPerCluster = 0;
	  ulong bytesPerSector = 0;
	  ulong numberOfFreeClusters = 0;
	  ulong totalNumberOfClusters = 0;
	  isAccessible = GetDiskFreeSpace(driveRoot,
					  &sectorsPerCluster,
					  &bytesPerSector,
					  &numberOfFreeClusters,
					  &totalNumberOfClusters);
	  if (isAccessible )
	    {
	      totalBytes = bytesPerSector*
		sectorsPerCluster*
		totalNumberOfClusters;
	      totalKBytes = bytesPerSector*
		(sectorsPerCluster*
		 totalNumberOfClusters)/1024;
	      availableBytes = bytesPerSector*
		sectorsPerCluster*
		numberOfFreeClusters;
	      availableKBytes = bytesPerSector*
		(sectorsPerCluster*
		 numberOfFreeClusters)/1024;
	    }
	}

#  endif
   return availableBytes;
   }

S_DiskDrive::~S_DiskDrive() {
   }


#ifndef S_DISKDRIVE
#define S_DISKDRIVE

class S_DiskDrive {
public:
   int index;
   char letter;

   int isAccessible;

   ulong totalBytes;
   ulong totalKBytes;
   ulong availableBytes;
   ulong availableKBytes;

   int Update();

   S_DiskDrive(int);
   ~S_DiskDrive();
   };

#endif

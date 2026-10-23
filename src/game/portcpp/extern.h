/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
/* ************************************ */
/* ***** Externals from Doug's code *** */
/* ************************************ */
extern char frameRate;
extern slong frameNumber;
extern short localSlotNumber;
extern SF_Client *sf;
/* ************************************ */

extern SF_MsgBuffer retMsgBuffer;

extern short warningMessagePosted;
extern slong warningMessagePostedFrame;
extern short warningBottomMessagePosted;
extern slong warningBottomMessagePostedFrame;

extern short numPlanetRobotsSet;
extern short numPopRobotsSet;

extern int inServer;
extern char instanceName[];

extern char message[MESSAGE_BUFFER_SIZE][MESSAGE_STRING_LENGTH+1];
extern char messagePushed;
void SetScreenMessage( char *, int );



/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
/*** Macros ***/

#define DEF_OBJ_PUSHTO   slong PushTo(class GameStateBuffer&)
#define DEF_OBJ_POPFROM  slong PopFrom(class GameStateBuffer&)
#define DEF_GAMEOBJ_PUSHTO   slong PushTo(class GameStateBuffer&, class GameStateClass*)
#define DEF_GAMEOBJ_POPFROM  slong PopFrom(class GameStateBuffer&, class GameStateClass*)

#define CALL_PUSH_GAME_OBJ(gb,gs,obj) { /*long n = */gb.Push(obj,gs); /*WRITE_SF_LOG(logFile << "pushing " #obj << " (" << n << " bytes)" << endl);*/ }
#define CALL_POP_GAME_OBJ(gb,gs,obj) { /*long n = */gb.Pop(obj,gs); /*WRITE_SF_LOG(logFile << "popping " #obj << " (" << n << " bytes)" << endl);*/ }
#define CALL_PUSH_OBJ(gb,obj) { /*long n = */gb.Push(obj); /*WRITE_SF_LOG(logFile << "pushing " #obj <<  " (" << n << " bytes)" << endl);*/ }
#define CALL_POP_OBJ(gb,obj) { /*long n = */gb.Pop(obj); /*WRITE_SF_LOG(logFile << "popping " #obj << " (" << n << " bytes)" << endl); */}

/********************************************************************************/
/* To checksum protect a class from RAM cheating, include the following macro   */
/* calls for each relevant data type and define these member functions          */
/*                                                                              */
/* void  HandleCheckSumFail() // to handle a failed checksum condition          */
/* void ClassCheck()          // to check or correct any additional constraints */
/* unsigned long CheckSum()   // returns CheckSum of all protected variables    */
/*                                                                              */
/********************************************************************************/

void HandleCheckSumFail();

#define DEF_MULT_TO_STAT(t) void MultToStat(t& p, t inc) {if ( checkSum != 0 && checkSum != CheckSum() ) HandleCheckSumFail(); ClassCheck(); p *= inc; checkSum = CheckSum(); }
#define DEF_ADD_TO_STAT(t) void AddToStat(t& p, t inc) {if ( checkSum != 0 && checkSum != CheckSum() ) HandleCheckSumFail(); ClassCheck(); p += inc; checkSum = CheckSum(); }
#define DEF_SET_STAT(t) void SetStat(t& p, t val) {if ( checkSum != 0 && checkSum != CheckSum()) HandleCheckSumFail(); p = val; ClassCheck(); checkSum = CheckSum(); }

#define MESSAGE_BUFFER_SIZE   1024
#define MESSAGE_STRING_LENGTH 38

#define FROM_GOD       255
#define FROM_RACE0     254
#define FROM_RACE1     253
#define FROM_RACE2     252
#define FROM_RACE3     251

#define TO_BROADCAST   255
#define TO_RACE0       254
#define TO_RACE1       253
#define TO_RACE2       252
#define TO_RACE3       251

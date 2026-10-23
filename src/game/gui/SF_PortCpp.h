/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_PORTCPP_INCLUDED
#define SF_PORTCPP_INCLUDED

/* ******************************************************************* */
/* ***** These includes are necessary for the enginepp func defs ***** */
/* ***** Eventually I'd like to see them go away                 ***** */
/* ******************************************************************* */
#if defined(OS2) || defined(WIN32)
#  include "../portcpp/classes.h"
#  include "../portcpp/enginepp.h"
#  include "../portcpp/enum.h"
#  include "../portcpp/const.h"
#  include "../portcpp/macros.h"
#  include "../portcpp/extern_portcpp.h"
#  include "../portcpp/sound.h"
#  include "../portcpp/trek.h"
#endif
#ifdef UNIX
#  include "../portcpp/classes.h"
#  include "../portcpp/enginepp.h"
#  include "../portcpp/enum.h"
#  include "../portcpp/const.h"
#  include "../portcpp/macros.h"
#  include "../portcpp/extern_portcpp.h"
#  include "../portcpp/sound.h"
#  include "../portcpp/trek.h"
#endif
/* *************************************************************** */

#endif



#if defined(WIN32) || defined(_WIN32)
/*Compatibility*/

#ifdef USE_TERMIOS
#undef USE_TERMIOS
#endif

#endif

#ifdef USE_AV
/*WARNING! Underdeveloped!*/
#include "d_av.h"

#else

#ifdef USE_TERMIOS
#include "d_termios.h"
#else
#include "d_simple.h"
#endif

#endif

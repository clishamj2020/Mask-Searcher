#ifndef ASSERT_H
#define ASSERT_H

//--------------------------------------------------------------------
//
// Copyright (C) 2022 raodm@miamiOH.edu
//
// Miami University makes no representations or warranties about the
// suitability of the software, either express or implied, including
// but not limited to the implied warranties of merchantability,
// fitness for a particular purpose, or non-infringement.  Miami
// University shall not be liable for any damages suffered by licensee
// as a result of using, result of using, modifying or distributing
// this software or its derivatives.
//
// By using or copying this Software, Licensee agrees to abide by the
// intellectual property laws, and all other applicable laws of the
// U.S., and the terms of GNU General Public License (version 3).
//
// Authors:   DJ Rao          raodm@miamiOH.edu
//
//---------------------------------------------------------------------

#ifndef ASSERT
#ifdef DEVELOPER_ASSERTIONS
#include <assert.h>
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif
#endif

#endif

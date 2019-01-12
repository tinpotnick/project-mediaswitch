

#ifndef PROJECTSIPDIRECTORY_H
#define PROJECTSIPDIRECTORY_H

#include "projectwebdocument.h"

/*******************************************************************************
File: projectsipdirectory.h/cpp.
Purpose: Store and lookup actualy username and passwords. Cache the results
in a fast lookup memory structure.
*******************************************************************************/


/*******************************************************************************
Class: projectsipdirectory
Purpose: Lookup and store domain, username and password information for our UACs.
Updated: 09.01.2019
*******************************************************************************/
class projectsipdirectory
{

  static void lookup( substring domain, substring user );
};

#endif /* PROJECTSIPDIRECTORY_H */


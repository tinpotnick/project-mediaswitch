


#include "projectsipdirectory.h"


/*******************************************************************************
Function: lookup
Purpose: Looks up the users password from our directory. The call is 
asynchronous as we may have to look elsewhere for our answer.
Updated: 15.01.2019
*******************************************************************************/
void projectsipdirectory::lookup( substring domain, substring user, std::function<void ( stringptr ) > callback )
{
  stringptr result( new std::string( "1123654789" ) );
  callback( result );
}

#include "json.hpp"
#include "projectsipstring.h"

#ifndef PROJECTSIPSDP_H
#define PROJECTSIPSDP_H

/*******************************************************************************
Purpose: Utilities to convert SDP to and FROM JSON structures.
Updated: 31.01.2019
*******************************************************************************/
bool sdptojson( substring in, JSON::Value &out );
stringptr jsontosdp( JSON::Object &in );



#endif /* PROJECTSIPSDP_H */


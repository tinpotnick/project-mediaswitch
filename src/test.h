


#ifndef PROJECTTEST_H
#define PROJECTTEST_H

#ifdef TESTCODE

/*******************************************************************************
Function: test
Purpose: Macro to call a funtion and test the result and display accordingly.
Updated: 13.12.2018
*******************************************************************************/
#define projecttest(s, d, e) if( s != d ){ std::cout << __FILE__ << ":" << __LINE__ << " " << e << " '" << s << "' != '" << d << "'" << std::endl; return; }

#define projecttestp(s, d, e) if( s != d ){ std::cout << __FILE__ << ":" << __LINE__ << " " << e << std::endl; return; }


#endif /* TESTCODE */

#endif /* PROJECTTEST_H */






#include <string>


#ifndef PROJECTSIPCONFIG_H
#define PROJECTSIPCONFIG_H


class projectsipconfig
{
public:
  projectsipconfig();
  static const char* gethostname( void );
  static const char* gethostip( void );
  static const int getsipport( void );

private:
  std::string hostname;
};

#endif /* PROJECTSIPCONFIG_H */


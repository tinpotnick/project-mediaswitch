

#include <string>


#ifndef PROJECTSIPCONFIG_H
#define PROJECTSIPCONFIG_H


class projectsipconfig
{
public:
  projectsipconfig();
  static const char* gethostip( void );
  static const short getsipport( void );
  static const char* gethostipsipport( void );

  static const void setsipport( short );
  static const void setsiphostip( std::string );

private:
  std::string hostip;
  short sipport;
  std::string hostipsipport;
};

#endif /* PROJECTSIPCONFIG_H */

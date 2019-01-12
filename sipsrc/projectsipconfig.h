

#include <string>


#ifndef PROJECTSIPCONFIG_H
#define PROJECTSIPCONFIG_H


class projectsipconfig
{
public:
  projectsipconfig();
  const char* gethostname( void );

private:
  std::string hostname;
};

#ifndef PROJECTSIPCONFIGEXTERN
extern projectsipconfig cnf;
#endif

#endif /* PROJECTSIPCONFIG_H */


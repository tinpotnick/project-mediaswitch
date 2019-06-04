

#ifndef PROJECTSIPGLOBALS_H
#define PROJECTSIPGLOBALS_H

#include <boost/asio.hpp>
#include "projectsipserver.h"

/* Try not to use too many globals. Keep track of ones we do here */
extern boost::asio::io_service io_service;
extern projectsipserver::pointer sipserver;

#endif /* PROJECTSIPGLOBALS_H */

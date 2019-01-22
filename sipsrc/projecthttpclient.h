

#ifndef PROJECTHTTPCLIENT_H
#define PROJECTHTTPCLIENT_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <functional>

#include "projectwebdocument.h"

/*
TODO 

1. Get the io context within this file. Anyone file using
a webclient doens't need to know about io contexts.

2. The callback shoud supply the doument.

*/

/*******************************************************************************
Class: projecthttpclient
Purpose: Simple async HTTP Client.
Updated: 18.01.2019
*******************************************************************************/
class projecthttpclient :
  public boost::enable_shared_from_this< projecthttpclient >
{
public:
  projecthttpclient();
  typedef boost::shared_ptr< projecthttpclient > pointer;
  static pointer create( boost::asio::io_context& iocontext );

  projecthttpclient( boost::asio::io_service &ioservice );

  void asyncrequest( projectwebdocumentptr request, 
      std::function< void ( boost::system::error_code errorcode ) > );

  projectwebdocumentptr getresponse( void );

private:
  boost::asio::io_service &ioservice;
  boost::asio::ip::tcp::socket socket;
  std::function< void ( boost::system::error_code errorcode ) > callback;
  stringptr requestdoc;
  projectwebdocumentptr response;

  std::string inbounddata;
};

#endif /* PROJECTHTTPCLIENT_H */


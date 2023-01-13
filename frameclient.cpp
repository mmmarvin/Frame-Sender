#include "frameclient.h"

#include <cstdio>
using boost::asio::ip::tcp;

FrameClient::FrameClient() :
  m_connectionresolver(get_context())
{
}

void FrameClient::connect(std::string_view ip,
                          port_t port,
                          connect_completion_t completion)
{
  boost::asio::async_connect(get_connection_socket(),
                             m_connectionresolver.resolve(ip, std::to_string(port)),
  [this, completion = std::move(completion)](boost::system::error_code ec,
                                             tcp::endpoint ep)
  {
    if(ec)
    {
      printf("Connection failed.\n");
      return;
    }

    printf("Connected to server %s.\n", ep.address().to_v4().to_string().c_str());
    completion();
  });
}

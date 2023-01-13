#ifndef FRAMECLIENT_H
#define FRAMECLIENT_H

#include <boost/asio.hpp>
#include "framebase.h"

class FrameClient : public FrameBase
{
  using connect_completion_t = std::function<void()>;

public:
  FrameClient();

  void connect(std::string_view ip, port_t port, connect_completion_t completion);

private:
  boost::asio::ip::tcp::resolver  m_connectionresolver;
};

#endif // FRAMECLIENT_H

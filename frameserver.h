#ifndef FRAMESERVER_H
#define FRAMESERVER_H

#include "framebase.h"

class FrameServer : public FrameBase
{
public:
  FrameServer(port_t connection_port);

  void run(std::function<void()> acceptor_complete = std::function<void()>());
  void set_acceptor_completion(std::function<void()> complete);

private:
  void start_acceptor();
  virtual void close_connection_and_restart() override;

  boost::asio::ip::tcp::acceptor  m_connectionacceptor;
  std::function<void()>           m_acceptorcomplete;
};

#endif // FRAMESERVER_H

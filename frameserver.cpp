#include "frameserver.h"

#include <cstdio>
using boost::asio::ip::tcp;

FrameServer::FrameServer(port_t connection_port) :
  m_connectionacceptor(get_context(), tcp::endpoint(tcp::v4(), connection_port))
{
}

void FrameServer::run(std::function<void()> acceptor_complete)
{
  FrameBase::run();

  if(acceptor_complete && !m_acceptorcomplete)
  {
    set_acceptor_completion(std::move(acceptor_complete));
  }
  start_acceptor();
}

void FrameServer::set_acceptor_completion(std::function<void()> complete)
{
  m_acceptorcomplete = std::move(complete);
}

void FrameServer::start_acceptor()
{
  if(get_connection_socket().is_open())
  {
    return;
  }

  m_connectionacceptor.async_accept(
  [this](boost::system::error_code ec, tcp::socket sock)
  {
    if(ec)
    {
      start_acceptor();
      return;
    }

    printf("Client %s connected.\n", sock.remote_endpoint().address().to_v4().to_string().c_str());
    get_connection_socket() = std::move(sock);
    if(m_acceptorcomplete)
    {
      m_acceptorcomplete();
    }
  });
}

void FrameServer::close_connection_and_restart()
{
  FrameBase::close_connection_and_restart();
  start_acceptor();
}

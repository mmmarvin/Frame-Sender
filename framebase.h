#ifndef FRAMEBASE_H
#define FRAMEBASE_H

#include <list>
#include <boost/asio.hpp>

class FrameBase
{
public:
  using port_t            = boost::asio::ip::port_type;
  using binary_t          = unsigned char;
  using frame_t           = std::vector<binary_t>;

private:
  using send_completion_t = std::function<void()>;
  using read_completion_t = std::function<void(const binary_t*, std::size_t)>;

public:
  FrameBase();
  ~FrameBase();

  void run();
  virtual void stop();

  void send_frame(frame_t frame, send_completion_t completion);
  void read_frame(read_completion_t completion);

  void send_data(const frame_t& data, send_completion_t completion);
  void read_data(read_completion_t completion);

  void set_frame_skip_count(std::size_t count);
  void set_frame_queue_threshold(std::size_t threshold);
  void set_read_buffer_size(std::size_t size);

protected:
  boost::asio::io_context& get_context() { return m_ioc; }
  const boost::asio::io_context& get_context() const { return m_ioc; }

  boost::asio::ip::tcp::socket& get_connection_socket() { return m_connectionsocket; }
  const boost::asio::ip::tcp::socket& get_connection_socket() const { return m_connectionsocket; }

  virtual void close_connection_and_restart();

private:
  void start_writer();

  struct Frame
  {
    frame_t           data;
    send_completion_t completion;
  };

  boost::asio::io_context         m_ioc;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
                                  m_iocwg;
  std::thread                     m_iocthread;

  boost::asio::ip::tcp::socket    m_connectionsocket;

  std::list<Frame>                m_framequeue;
  std::atomic_uint64_t            m_framequeuesize;
  std::size_t                     m_framequeuesizethreshold;

  std::size_t                     m_frameskip;
  std::size_t                     m_frameskipcount;

  frame_t                         m_readbuffer;

  bool                            m_running;
};

FrameBase::frame_t string_to_frame(std::string_view str);
FrameBase::frame_t string_to_frame(const char* str, std::size_t str_length);

#endif // FRAMEBASE_H

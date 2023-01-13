#include "framebase.h"

#include <cstdio>
using boost::asio::ip::tcp;


FrameBase::FrameBase() :
  m_iocwg(boost::asio::make_work_guard(m_ioc)),

  m_connectionsocket(m_ioc),

  m_framequeuesize(0),
  m_framequeuesizethreshold(10),

  m_frameskip(0),
  m_frameskipcount(0)
{
}

FrameBase::~FrameBase()
{
  stop();
}

void FrameBase::run()
{
  m_running = true;
  m_ioc.restart();

  m_iocthread = std::thread([this]() { m_ioc.run(); });
}

void FrameBase::stop()
{
  if(m_running)
  {
    boost::asio::post(m_ioc, [this]()
    {
      if(m_connectionsocket.is_open())
      {
        m_connectionsocket.cancel();
        m_connectionsocket.close();
      }

      m_ioc.stop();
    });
    m_iocthread.join();
    m_running = false;
  }
}

void FrameBase::send_frame(frame_t frame, send_completion_t completion)
{
  if(frame.empty() || !m_connectionsocket.is_open())
  {
    return;
  }

  if(m_frameskipcount)
  {
    if(m_frameskip < m_frameskipcount)
    {
      ++m_frameskip;
      return;
    }
    else
    {
      m_frameskip = 0;
    }
  }

  auto fqueue_size = m_framequeuesize.load();
  if(fqueue_size < m_framequeuesizethreshold)
  {
    boost::asio::post(m_ioc, [this, frame = std::move(frame), completion = std::move(completion)]()
    {
      if(m_framequeue.size() < m_framequeuesizethreshold)
      {
        auto write_in_progress = !m_framequeue.empty();

        m_framequeue.push_back({ std::move(frame), std::move(completion) });
        m_framequeuesize.store(m_framequeuesize + 1);

        if(!write_in_progress)
        {
          start_writer();
        }
      }
    });
  }
}

void FrameBase::read_frame(read_completion_t completion)
{
  assert(m_readbuffer.size());
  if(!m_connectionsocket.is_open())
  {
    return;
  }

  m_connectionsocket.async_read_some(boost::asio::buffer(m_readbuffer.data(), m_readbuffer.size()),
  [this, completion = std::move(completion)](boost::system::error_code ec,
                                             std::size_t read)
  {
    if(ec)
    {
      close_connection_and_restart();
      return;
    }

    if(read)
    {
      completion(m_readbuffer.data(), read);
    }
  });
}

void FrameBase::send_data(const frame_t& data, send_completion_t completion)
{
  if(data.empty() || !m_connectionsocket.is_open())
  {
    return;
  }

  boost::asio::async_write(m_connectionsocket,
                           boost::asio::buffer(data.data(), data.size()),
  [this, completion = std::move(completion)](boost::system::error_code ec, std::size_t sent)
  {
    if(ec)
    {
      close_connection_and_restart();
      return;
    }

    if(sent)
    {
      completion();
    }
  });
}

void FrameBase::read_data(read_completion_t completion)
{
  assert(m_readbuffer.size());
  if(!m_connectionsocket.is_open())
  {
    return;
  }

  m_connectionsocket.async_read_some(boost::asio::buffer(m_readbuffer.data(), m_readbuffer.size()),
  [this, completion = std::move(completion)](boost::system::error_code ec, std::size_t read)
  {
    if(ec)
    {
      close_connection_and_restart();
      return;
    }

    if(read)
    {
      completion(m_readbuffer.data(), read);
    }
  });
}

void FrameBase::set_frame_skip_count(std::size_t count)
{
  m_frameskipcount = count;
}

void FrameBase::set_frame_queue_threshold(std::size_t threshold)
{
  m_framequeuesizethreshold = threshold;
}

void FrameBase::set_read_buffer_size(std::size_t size)
{
  m_readbuffer.resize(size);
}

void FrameBase::close_connection_and_restart()
{
  if(m_connectionsocket.is_open())
  {
    printf("Connection closed.\n");
    m_connectionsocket.cancel();
    m_connectionsocket.close();
  }

  m_framequeue.clear();
  m_framequeuesize.store(0);
}

void FrameBase::start_writer()
{
  auto& front_frame = m_framequeue.front().data;
  boost::asio::async_write(m_connectionsocket,
                           boost::asio::buffer(front_frame.data(), front_frame.size()),
  [this, frame_size = front_frame.size()](boost::system::error_code ec, std::size_t sent)
  {
    if(ec || !sent)
    {
      close_connection_and_restart();
      return;
    }

    // call completion
    m_framequeue.front().completion();

    m_framequeue.pop_front();
    m_framequeuesize.store(m_framequeuesize.load() - 1);
    if(!m_framequeue.empty())
    {
      start_writer();
    }
  });
}

FrameBase::frame_t string_to_frame(std::string_view str)
{
  return string_to_frame(str.data(), str.size());
}

FrameBase::frame_t string_to_frame(const char* str, std::size_t str_length)
{
  FrameBase::frame_t ret;
  ret.insert(ret.end(), str, str + str_length);

  return ret;
}

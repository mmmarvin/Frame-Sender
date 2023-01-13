#include "image_publisher.h"

#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
#include "frameserver.h"

static FrameServer::frame_t image1;
static FrameServer::frame_t image2;

std::string numberToString(int number)
{
  assert(number < 100000);

  if(number < 10)
  {
    return "0000" + std::to_string(number);
  }
  else if(number < 100)
  {
    return "000" + std::to_string(number);
  }
  else if(number < 1000)
  {
    return "00" + std::to_string(number);
  }
  else if(number < 10000)
  {
    return "0" + std::to_string(number);
  }

  return std::to_string(number);
}

void load_images()
{
  sf::Image image;
  {
    image.loadFromFile("image1.png");
    auto number_of_bits = image.getSize().x * image.getSize().y * 4;

    std::string header = "EHLO" + numberToString(image.getSize().x) + "x" + numberToString(image.getSize().y);
    image1.resize(header.size() + number_of_bits);

    std::memcpy(image1.data(), header.data(), header.size());
    std::memcpy(image1.data() + header.size(), image.getPixelsPtr(), number_of_bits);
  }
  {
    image.loadFromFile("image2.png");
    auto number_of_bits = image.getSize().x * image.getSize().y * 4;

    std::string header = "EHLO" + numberToString(image.getSize().x) + "x" + numberToString(image.getSize().y);
    image2.resize(header.size() + number_of_bits);

    std::memcpy(image2.data(), header.data(), header.size());
    std::memcpy(image2.data() + header.size(), image.getPixelsPtr(), number_of_bits);
  }
}

void read_replies(FrameServer& frame_server)
{
  // we don't care what the client sends, we just want to keep the connection alive
  frame_server.read_data([&frame_server](const unsigned char*, std::size_t)
  {
    read_replies(frame_server);
  });
}

void run_publisher()
{
  load_images();

  FrameServer frame_server(8020);
  std::atomic_bool ready = false;

  frame_server.set_frame_queue_threshold(60);
  frame_server.set_frame_skip_count(2);
  frame_server.set_read_buffer_size(512);
  frame_server.set_acceptor_completion([&frame_server, &ready]()
  {
    printf("Initiating handshake...");
    frame_server.read_data(
    [&frame_server, &ready](const unsigned char* data, std::size_t data_length)
    {
      if(std::string(reinterpret_cast<const char*>(data), data_length) == "EHLO")
      {
        printf("Done.\n");
        frame_server.send_data(string_to_frame("Ok"), []() {});
        ready.store(true);

        read_replies(frame_server);
      }
      else
      {
        printf("There was an error in handshake. Exiting...\n");
        return;
      }
    });
  });
  frame_server.run();

  for(;;)
  {
    if(!ready.load())
    {
      continue;
    }

    frame_server.send_frame(image1, [](){});
    std::this_thread::sleep_for(std::chrono::milliseconds(int((1.f / 60.f) * 1000.f)));
    frame_server.send_frame(image2, [](){});
    std::this_thread::sleep_for(std::chrono::milliseconds(int((1.f / 60.f) * 1000.f)));
  }
}

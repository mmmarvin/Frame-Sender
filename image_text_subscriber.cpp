#include "image_text_subscriber.h"

#include <SFML/Graphics.hpp>
#include "frameclient.h"

static std::string         text;
static std::string         text_swap;
static std::mutex          text_mutex;
static std::atomic_bool    new_text = false;

static sf::Text            draw_text;

static void draw_sf_image(sf::RenderWindow& renderWindow)
{
  if(new_text.load())
  {
    {
      std::lock_guard<std::mutex> lock(text_mutex);
      text_swap = std::move(text);
    }

    draw_text.setString(text_swap);
    new_text.store(false);
  }

  renderWindow.draw(draw_text);
}

static void read_data(FrameClient& frame_subscriber)
{
  frame_subscriber.read_frame([&frame_subscriber](const unsigned char* data,
                                                  std::size_t data_length)
  {
    {
      std::lock_guard<std::mutex> lock(text_mutex);
      text = std::string(reinterpret_cast<const char*>(data), data_length);
    }

    new_text.store(true);
  });
}

void run_text_subscriber()
{
  FrameClient frame_client;
  frame_client.set_read_buffer_size(1024 * 20);
  frame_client.run();

  frame_client.connect("127.0.0.1", 8020, [&frame_client]()
  {
    printf("Initiating handshake...");
    frame_client.send_data(string_to_frame("EHLO"), [&frame_client]()
    {
      frame_client.read_data([&frame_client](const unsigned char* data,
                                             std::size_t data_length)
      {
        if(std::string(reinterpret_cast<const char*>(data), data_length) == "Ok")
        {
          printf("Done.\n");
          read_data(frame_client);
        }
        else
        {
          printf("There was an error in handshake. Exiting...\n");
          return;
        }
      });
    });
  });

  sf::RenderWindow renderWindow(sf::VideoMode(800, 600), "Test");
  sf::Font font;
  font.loadFromFile("DejaVuSans.ttf");
  draw_text.setFont(font);
  draw_text.setPosition(0, 0);

  while(renderWindow.isOpen())
  {
    sf::Event event;
    while(renderWindow.pollEvent(event))
    {
      switch(event.type)
      {
      case sf::Event::Closed:
        renderWindow.close();
        break;
      default:
        break;
      }
    }

    renderWindow.clear();
    draw_sf_image(renderWindow);
    renderWindow.display();
  }
}

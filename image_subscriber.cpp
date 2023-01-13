#include "image_subscriber.h"

#include <SFML/Graphics.hpp>
#include "frameclient.h"

static const std::string_view CONNECTION_HEADER = "EHLO";

static sf::Image                             draw_image;
static sf::Texture                           draw_texture;

static FrameClient::frame_t                  image;
static FrameClient::frame_t                  image_swap;
static sf::Vector2u                          image_dimension;
static sf::Vector2u                          image_dimension_swap;
static std::mutex                            image_mutex;
static std::atomic_bool                      new_text = false;

static FrameClient::frame_t                  temp_image;
static sf::Vector2u                          temp_image_dimension;
static std::size_t                           number_of_bytes;

static FrameClient::frame_t                  temp_buffer;

static sf::RectangleShape                    draw_rect;

static int fast_atoi(const char* first, const char* last)
{
    int val = 0;
    while(first != last) {
        val = val * 10 + (*first++ - '0');
    }
    return val;
}

static void draw_sf_image(sf::RenderWindow& renderWindow)
{
  if(new_text.load())
  {
    {
      std::lock_guard<std::mutex> lock(image_mutex);
      image_swap = std::move(image);
      image_dimension_swap = image_dimension;
    }

    draw_image.create(image_dimension_swap.x,
                      image_dimension_swap.y,
                      image_swap.data());

    draw_texture.loadFromImage(draw_image);
    draw_rect.setSize(sf::Vector2f(image_dimension.x, image_dimension.y));
    draw_rect.setTexture(&draw_texture);
    new_text.store(false);
  }

  renderWindow.draw(draw_rect);
}

static void read_data(FrameClient& frame_subscriber)
{
  frame_subscriber.read_frame([&frame_subscriber](const unsigned char* data,
                                                  std::size_t data_length)
  {
    temp_buffer.insert(temp_buffer.end(), data, data + data_length);
    if(temp_buffer.size() >= 15)
    {
      auto* temp_buffer_data = reinterpret_cast<char*>(temp_buffer.data());
      auto* temp_buffer_data_end = temp_buffer_data + temp_buffer.size();
      if(temp_image.empty())
      {
        if(!strncmp(temp_buffer_data, CONNECTION_HEADER.data(), CONNECTION_HEADER.size()))
        {
          auto dx = fast_atoi(temp_buffer_data + CONNECTION_HEADER.size(),
                              temp_buffer_data + CONNECTION_HEADER.size() + 5);
          auto dy = fast_atoi(temp_buffer_data + CONNECTION_HEADER.size() + 6,
                              temp_buffer_data + CONNECTION_HEADER.size() + 11);
          number_of_bytes = dx * dy * 4;
          temp_image_dimension = sf::Vector2u(dx, dy);

          if(temp_buffer.size() > CONNECTION_HEADER.size() + 11)
          {
            temp_image.insert(temp_image.end(),
                              temp_buffer_data + CONNECTION_HEADER.size() + 11,
                              temp_buffer_data_end);
          }
          temp_buffer.clear();
        }
      }
      else
      {
        if(temp_image.size() + temp_buffer.size() > number_of_bytes)
        {
          auto header_position = std::find_first_of(temp_buffer_data,
                                                    temp_buffer_data_end,
                                                    CONNECTION_HEADER.data(),
                                                    CONNECTION_HEADER.data() +
                                                    CONNECTION_HEADER.size());
          if(header_position != temp_buffer_data_end)
          {
            temp_image.insert(temp_image.end(), temp_buffer_data, header_position);
            {
              std::lock_guard<std::mutex> lock(image_mutex);
              image = std::move(temp_image);
              image_dimension = temp_image_dimension;
            }
            frame_subscriber.send_data(string_to_frame("REOK"), [](){});

            new_text.store(true);
            temp_image.clear();

            FrameClient::frame_t temp;
            temp.insert(temp.end(), header_position, temp_buffer_data_end);
            temp_buffer = std::move(temp);
          }
          else
          {
            // corrupted data
            // just copy everything and hope for the best
            temp_image.insert(temp_image.end(), temp_buffer_data, temp_buffer_data_end);
            {
              std::lock_guard<std::mutex> lock(image_mutex);
              image = std::move(temp_image);
              image_dimension = temp_image_dimension;
            }
            frame_subscriber.send_data(string_to_frame("REOK"), [](){});

            new_text.store(true);
            temp_image.clear();
            temp_buffer.clear();
          }
        }
        else
        {
          temp_image.insert(temp_image.end(), temp_buffer_data, temp_buffer_data_end);
          if(temp_image.size() == number_of_bytes)
          {
            {
              std::lock_guard<std::mutex> lock(image_mutex);
              image = std::move(temp_image);
              image_dimension = temp_image_dimension;
            }
            frame_subscriber.send_data(string_to_frame("REOK"), [](){});

            new_text.store(true);
            temp_image.clear();
          }
          temp_buffer.clear();
        }
      }
    }

    read_data(frame_subscriber);
  });
}

void run_subscriber()
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

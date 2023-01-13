#include <cstdio>
#include <filesystem>
#include <thread>
#include "frameserver.h"
#include "image_publisher.h"
#include "image_subscriber.h"
#include "image_text_subscriber.h"

void print_usage(std::string prog)
{
  printf("%s --[publisher/subscriber]", prog.c_str());
}

int main(int argc, char** argv)
{
  if(argc == 2)
  {
    std::string option = std::string(argv[1]);
    if(option == "--subscriber")
    {
      printf("Running subscriber...\n");
      run_subscriber();
    }
    else if(option == "--publisher")
    {
      printf("Running publisher...");
      run_publisher();
    }
    else if(option == "--text-subscriber")
    {
      printf("Running text subscriber...");
      run_text_subscriber();
    }

    return 0;
  }

  printf("Invalid usage: ");
  print_usage(argv[0]);
  return -1;
}

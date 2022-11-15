#ifndef beams_window_h
#define beams_window_h

#include "Result.h"

#include <memory>
#include <string>

namespace beams
{

class Window
{
public:
  Window();
  ~Window();

  Result Initialize(std::string title, int width, int height);

  void Run();

  void Terminate();

  struct InternalsType;

private:
  std::shared_ptr<InternalsType> Internals;
}; // class Window

} // namespace beams

#endif // beams_window_h
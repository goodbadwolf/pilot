#include "Window.h"
#include "Result.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include <iostream>
#include <string>

namespace beams
{

struct Window::InternalsType
{
  InternalsType() = default;

  InternalsType(const char* title, int width, int height)
    : Title(title)
    , Width(width)
    , Height(height)
    , GlfwInitialized(false)
    , Window(nullptr)
    , GLSL_Version()
    , GlewInitialized(false)
    , ImGuiInitialized(false)
    , ImGuiCtx(nullptr)
  {
  }

  bool Initialized() const
  {
    return this->GlfwInitialized && this->GlewInitialized && this->ImGuiInitialized;
  }

  std::string Title;
  int Width;
  int Height;
  bool GlfwInitialized;
  GLFWwindow* Window;
  std::string GLSL_Version;
  bool GlewInitialized;
  bool ImGuiInitialized;
  ImGuiContext* ImGuiCtx;
}; // struct Window::InternalsType

} // namespace beams

namespace
{
beams::Result InitializeGlfw(std::shared_ptr<beams::Window::InternalsType> internals)
{
  auto errorCallback = [](int error, const char* description)
  { fmt::print("Glfw error: '{}' with description: '{}' occured\n", error, description); };

  glfwSetErrorCallback(errorCallback);

  if (!glfwInit())
  {
    internals->GlfwInitialized = false;
    return beams::Result{ .Success = false, .Err = "Glfw failed to initialize" };
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow* window =
    glfwCreateWindow(internals->Width, internals->Height, internals->Title.c_str(), NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // enable vsync

  internals->GlfwInitialized = true;
  internals->GLSL_Version = "#version 150";
  internals->Window = window;

  return beams::Result{ .Success = true };
}

void TerminateGlfw(std::shared_ptr<beams::Window::InternalsType> internals)
{
  if (!internals->GlfwInitialized)
  {
    return;
  }

  glfwDestroyWindow(internals->Window);
  glfwTerminate();
  internals->GlfwInitialized = false;
}

beams::Result InitializeGlew(std::shared_ptr<beams::Window::InternalsType> internals)
{
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    internals->GlewInitialized = false;
    return beams::Result{ .Success = false,
                          .Err =
                            fmt::format("Error initializing glew: {}",
                                        reinterpret_cast<const char*>(glewGetErrorString(err))) };
  }

  internals->GlewInitialized = true;
  return beams::Result{ .Success = true };
}

void TerminateGlew(std::shared_ptr<beams::Window::InternalsType> internals)
{
  internals->GlewInitialized = false;
}

beams::Result InitializeImGui(std::shared_ptr<beams::Window::InternalsType> internals)
{
  IMGUI_CHECKVERSION();
  internals->ImGuiCtx = ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(internals->Window, true);
  ImGui_ImplOpenGL3_Init(internals->GLSL_Version.c_str());
  internals->ImGuiInitialized = true;

  return beams::Result{ .Success = true };
}

void TerminateImGui(std::shared_ptr<beams::Window::InternalsType> internals)
{
  if (!internals->ImGuiInitialized)
  {
    return;
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext(internals->ImGuiCtx);
  internals->ImGuiInitialized = false;
}
}

namespace beams
{

Window::Window()
  : Internals(new InternalsType())
{
}

Window::~Window() {}

Result Window::Initialize(std::string title, int width, int height)
{
  if (this->Internals->Initialized())
  {
    return Result{ .Success = true };
  }

  this->Internals->Title = title;
  this->Internals->Width = width;
  this->Internals->Height = height;

  auto result = InitializeGlfw(this->Internals);
  if (!result.Success)
  {
    return Result{ .Success = false,
                   .Err = fmt::format("Window initialization failed: {}", result.Err) };
  }

  result = InitializeGlew(this->Internals);
  if (!result.Success)
  {
    return Result{ .Success = false,
                   .Err = fmt::format("Window initialization failed: {}", result.Err) };
  }

  result = InitializeImGui(this->Internals);
  if (!result.Success)
  {
    return Result{ .Success = false,
                   .Err = fmt::format("Window initialization failed: {}", result.Err) };
  }

  return Result{ .Success = true };
}

void Window::Run()
{
  while (!glfwWindowShouldClose(this->Internals->Window))
  {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    ImGui::Render();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    int display_w, display_h;
    glfwGetFramebufferSize(this->Internals->Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w,
                 clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(this->Internals->Window);
  }

  this->Terminate();
}

void Window::Terminate()
{
  TerminateImGui(this->Internals);
  TerminateGlfw(this->Internals);
}

}

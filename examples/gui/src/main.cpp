// Interactive SiPM simulator.
//
// A generator thread fills a FIFO with photon events, a pool of SiPMSensor
// workers simulates them into a second FIFO, and the GUI thread shows the
// waveforms on an oscilloscope together with peak / integral / ToT / ToA
// histograms or the matrix of fired cells.
//
// Build:  cmake -B build -DSIPM_BUILD_GUI=ON && cmake --build build -j
// Run:    ./build/examples/gui/sipm_gui

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include <GLFW/glfw3.h>

#include "App.h"
#include "Png.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

namespace {

void applyTheme(float scale) {
  ImGuiStyle& s = ImGui::GetStyle();
  ImGui::StyleColorsDark(&s);
  s.WindowPadding = ImVec2(10, 10);
  s.FramePadding = ImVec2(8, 4);
  s.ItemSpacing = ImVec2(8, 6);
  s.ItemInnerSpacing = ImVec2(6, 4);
  s.WindowRounding = 0;
  s.ChildRounding = 6;
  s.FrameRounding = 4;
  s.PopupRounding = 6;
  s.GrabRounding = 4;
  s.ScrollbarRounding = 6;
  s.ScrollbarSize = 12;
  s.WindowBorderSize = 0;
  s.ChildBorderSize = 1;
  s.SeparatorTextBorderSize = 1;
  s.SeparatorTextPadding = ImVec2(0, 6);

  ImVec4* c = s.Colors;
  const ImVec4 accent(0.30f, 0.64f, 1.00f, 1.00f);
  c[ImGuiCol_Text] = ImVec4(0.88f, 0.90f, 0.93f, 1.00f);
  c[ImGuiCol_TextDisabled] = ImVec4(0.52f, 0.56f, 0.62f, 1.00f);
  c[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.085f, 0.10f, 1.00f);
  c[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
  c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.98f);
  c[ImGuiCol_Border] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
  c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
  c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
  c[ImGuiCol_FrameBgActive] = ImVec4(0.21f, 0.24f, 0.29f, 1.00f);
  c[ImGuiCol_Button] = ImVec4(0.17f, 0.19f, 0.23f, 1.00f);
  c[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.25f, 0.30f, 1.00f);
  c[ImGuiCol_ButtonActive] = ImVec4(accent.x, accent.y, accent.z, 0.60f);
  c[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
  c[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  c[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.50f);
  c[ImGuiCol_CheckMark] = accent;
  c[ImGuiCol_SliderGrab] = accent;
  c[ImGuiCol_SliderGrabActive] = accent;
  c[ImGuiCol_Separator] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
  c[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
  c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.55f);

  s.ScaleAllSizes(scale);
  s.FontScaleDpi = scale;

  ImPlotStyle& p = ImPlot::GetStyle();
  ImPlot::StyleColorsDark(&p);
  p.Colors[ImPlotCol_FrameBg] = ImVec4(0, 0, 0, 0);
  p.Colors[ImPlotCol_PlotBg] = ImVec4(0.035f, 0.045f, 0.055f, 1.00f);
  p.Colors[ImPlotCol_PlotBorder] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
  p.Colors[ImPlotCol_AxisGrid] = ImVec4(1, 1, 1, 0.07f);
  p.Colors[ImPlotCol_AxisText] = ImVec4(0.62f, 0.66f, 0.72f, 1.00f);
  p.PlotPadding = ImVec2(6 * scale, 6 * scale);
  p.MinorAlpha = 0.5f;
}

} // namespace

int main() {
  glfwSetErrorCallback([](int code, const char* desc) { std::fprintf(stderr, "GLFW error %d: %s\n", code, desc); });
  if (!glfwInit()) {
    return 1;
  }

  // Prefer a 3.3 core context, fall back to whatever the driver offers
  const char* glsl = "#version 330";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  const float scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
  const int w = static_cast<int>(1600 * scale), h = static_cast<int>(960 * scale);
  GLFWwindow* window = glfwCreateWindow(w, h, "SimSiPM - Interactive Simulator", nullptr, nullptr);
  if (window == nullptr) {
    glfwDefaultWindowHints();
    glsl = "#version 130";
    window = glfwCreateWindow(w, h, "SimSiPM - Interactive Simulator", nullptr, nullptr);
  }
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  applyTheme(scale);

  std::error_code ec;
  if (std::filesystem::exists(SIPM_GUI_FONT, ec)) {
    io.Fonts->AddFontFromFileTTF(SIPM_GUI_FONT, 15.0f);
  } else {
    io.Fonts->AddFontDefault();
  }

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl);

  {
    sipmgui::App app;
    int swapInterval = -1;
    auto frameStart = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
        ImGui_ImplGlfw_Sleep(10);
        continue;
      }

      // "Max" update speed renders as fast as possible; otherwise vsync plus a
      // software cap, since some compositors ignore the swap interval.
      const bool maxFps = app.maxFps();
      if (swapInterval != (maxFps ? 0 : 1)) {
        swapInterval = maxFps ? 0 : 1;
        glfwSwapInterval(swapInterval);
      }

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      app.frame();
      ImGui::Render();

      int fbw, fbh;
      glfwGetFramebufferSize(window, &fbw, &fbh);
      glViewport(0, 0, fbw, fbh);
      glClearColor(0.075f, 0.085f, 0.10f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      // PNG export: read back the plot region drawn this frame in print style
      float rect[4];
      std::string capturePath;
      if (app.takeCapture(rect, capturePath)) {
        const ImVec2 sc = ImGui::GetIO().DisplayFramebufferScale;
        const int x0 = std::max(0, static_cast<int>(rect[0] * sc.x)),
                  y0 = std::max(0, static_cast<int>(rect[1] * sc.y));
        const int x1 = std::min(fbw, static_cast<int>(rect[2] * sc.x)),
                  y1 = std::min(fbh, static_cast<int>(rect[3] * sc.y));
        const int w = std::max(1, x1 - x0), h = std::max(1, y1 - y0);
        std::vector<unsigned char> px(size_t(w) * h * 3), flipped(px.size());
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(x0, fbh - y1, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
        for (int y = 0; y < h; ++y) {
          std::copy_n(px.data() + size_t(h - 1 - y) * w * 3, size_t(w) * 3, flipped.data() + size_t(y) * w * 3);
        }
        app.captureDone(capturePath, sipmgui::writePng(capturePath, w, h, flipped));
      }
      glfwSwapBuffers(window);

      if (!maxFps) {
        constexpr auto kMinFrame = std::chrono::microseconds(1'000'000 / 120);
        const auto next = frameStart + kMinFrame;
        if (std::chrono::steady_clock::now() < next) {
          std::this_thread::sleep_until(next);
        }
      }
      frameStart = std::chrono::steady_clock::now();
    }
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

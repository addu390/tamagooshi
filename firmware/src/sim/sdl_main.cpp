#include <M5GFX.h>

#if defined(SDL_h_)

void setup();
void loop();

__attribute__((weak)) int user_func(bool* running) {
  setup();
  do {
    loop();
  } while (*running);
  return 0;
}

int main(int, char**) { return lgfx::Panel_sdl::main(user_func, 128); }

#endif

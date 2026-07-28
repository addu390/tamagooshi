#include "brand.gen.h"
#if defined(TAMA_ENABLE_PERSONA)

#include "mascot.h"
#include "persona.gen.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

class PersonaScreen : public AppScreen {
 public:
  const char* id() const override { return "persona"; }
  OrientationPref orientation() const override { return OrientationPref::Portrait; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "PERSONA");
    const auto& name = ctx.state.branding.persona_name;

    const int size = 66;
    const int mascotY = L.top + 32 + size / 2;
    const bool happy = (now() / 2200u) & 1u;
    characters::persona().draw(
        g, L.cx, mascotY, size,
        MascotState{happy ? Expr::Happy : Expr::Neutral, 0, false}, now());

    int y = widgets::mascotNameY(mascotY, size);
    if (!name.empty()) {
      g.str(name.c_str(), L.cx, y, theme::kHi, typeface::body(), textdatum_t::top_center);
      y += 18;
    }

    g.str(persona::kRole, L.cx, y, theme::kFg, typeface::body(), textdatum_t::top_center);
    y += 18;

    const widgets::ListItem rows[] = {
        {"JOINED", persona::kJoined},
    };
    widgets::infoList(g, L, y + 4, rows, 1);
    widgets::hints(g, "", "BACK");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    return Transition::none();
  }

  uint32_t redrawPeriodMs() const override { return 60; }
};

}  // namespace

TAMA_SCREEN_FACTORY(persona, PersonaScreen)

}  // namespace tama::screens

#endif  // TAMA_ENABLE_PERSONA

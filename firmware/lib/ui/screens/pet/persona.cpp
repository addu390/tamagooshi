#include "brand.gen.h"
#if defined(TAMA_ENABLE_PERSONA)

#include <algorithm>

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
    const auto L = widgets::frame(g, ctx.state, "ME");
    const auto& name = ctx.state.branding.persona_name;

    const int size = 66;
    const int aboutW = L.w - 16;
    const int aboutLines =
        persona::kAbout[0]
            ? std::min(static_cast<int>(widgets::wrap(g, persona::kAbout, aboutW,
                                                      typeface::body()).size()),
                       kAboutMaxLines)
            : 0;
    // The avatar reaches as far above its centre as the name sits below it, minus the gap.
    const int nameOffset = widgets::mascotNameY(0, size);
    const int avatarHalf = nameOffset - kNameGap;
    const int textH = (name.empty() ? 0 : kNameLineH) + aboutLines * kAboutLineH;
    const int blockH = textH > 0 ? avatarHalf + nameOffset + textH : 2 * avatarHalf;
    const int mascotY = L.top + (L.contentH() - blockH) / 2 + avatarHalf;

    const bool happy = (now() / 2200u) & 1u;
    characters::persona().draw(
        g, L.cx, mascotY, size,
        MascotState{happy ? Expr::Happy : Expr::Neutral, 0, false}, now());

    int y = widgets::mascotNameY(mascotY, size);
    if (!name.empty()) {
      g.str(name.c_str(), L.cx, y, theme::kHi, typeface::body(), textdatum_t::top_center);
      y += kNameLineH;
    }

    if (aboutLines > 0) {
      widgets::wrapText(g, persona::kAbout, L.cx, y, aboutW, typeface::body(), theme::kFg,
                        kAboutLineH, y + aboutLines * kAboutLineH - 1);
    }
    widgets::hints(g, "", "BACK");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    return Transition::none();
  }

  uint32_t redrawPeriodMs() const override { return 60; }

 private:
  static constexpr int kNameGap = 8;
  static constexpr int kNameLineH = 18;
  static constexpr int kAboutLineH = 16;
  static constexpr int kAboutMaxLines = 3;
};

}  // namespace

TAMA_SCREEN_FACTORY(persona, PersonaScreen)

}  // namespace tama::screens

#endif  // TAMA_ENABLE_PERSONA

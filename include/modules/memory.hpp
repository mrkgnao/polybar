#pragma once

#include <istream>

#include "config.hpp"
#include "modules/meta/timer_module.hpp"

POLYBAR_NS

namespace modules {
  enum class memtype { NONE = 0, TOTAL, USED, FREE, SHARED, BUFFERS, CACHE, AVAILABLE };

  class memory_module : public timer_module<memory_module> {
   public:
    using timer_module::timer_module;

    void setup();
    bool update();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR_USED = "<bar-used>";
    static constexpr auto TAG_BAR_FREE = "<bar-free>";

    label_t m_label;
    progressbar_t m_bar_free;
    map<memtype, progressbar_t> m_bars;
    map<memtype, int> m_perc;
  };
}

POLYBAR_NS_END

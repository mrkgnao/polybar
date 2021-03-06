#include "components/builder.hpp"

POLYBAR_NS

namespace modules {
  // module<Impl> public {{{

  template <typename Impl>
  module<Impl>::module(const bar_settings bar, const logger& logger, const config& config, string name)
        : m_bar(bar)
        , m_log(logger)
        , m_conf(config)
        , m_name("module/" + name)
        , m_builder(make_unique<builder>(bar))
        , m_formatter(make_unique<module_formatter>(m_conf, m_name)) {}

  template <typename Impl>
  module<Impl>::~module() noexcept {
    m_log.trace("%s: Deconstructing", name());

    for (auto&& thread_ : m_threads) {
      if (thread_.joinable()) {
        thread_.join();
      }
    }
  }

  template <typename Impl>
  void module<Impl>::set_update_cb(callback<>&& cb) {
    m_update_callback = forward<decltype(cb)>(cb);
  }

  template <typename Impl>
  void module<Impl>::set_stop_cb(callback<>&& cb) {
    m_stop_callback = forward<decltype(cb)>(cb);
  }

  template <typename Impl>
  string module<Impl>::name() const {
    return m_name;
  }

  template <typename Impl>
  bool module<Impl>::running() const {
    return m_enabled.load(std::memory_order_relaxed);
  }

  template <typename Impl>
  void module<Impl>::setup() {
    m_log.trace("%s: Setup", m_name);

    try {
      CAST_MOD(Impl)->setup();
    } catch (const std::exception& err) {
      m_log.err("%s: Setup failed", m_name);
      halt(err.what());
    }
  }

  template <typename Impl>
  void module<Impl>::stop() {
    if (!running()) {
      return;
    }

    m_log.info("%s: Stopping", name());
    m_enabled.store(false, std::memory_order_relaxed);

    wakeup();

    std::lock_guard<concurrency_util::spin_lock> guard(m_lock);
    {
      CAST_MOD(Impl)->teardown();

      if (m_mainthread.joinable()) {
        m_mainthread.join();
      }
    }

    if (m_stop_callback) {
      m_stop_callback();
    }
  }

  template <typename Impl>
  void module<Impl>::halt(string error_message) {
    m_log.err("%s: %s", name(), error_message);
    m_log.warn("Stopping '%s'...", name());
    stop();
  }

  template <typename Impl>
  void module<Impl>::teardown() {}

  template <typename Impl>
  string module<Impl>::contents() {
    return m_cache;
  }

  template <typename Impl>
  bool module<Impl>::handle_event(string cmd) {
    return CAST_MOD(Impl)->handle_event(cmd);
  }

  template <typename Impl>
  bool module<Impl>::receive_events() const {
    return false;
  }

  // }}}
  // module<Impl> protected {{{

  template <typename Impl>
  void module<Impl>::broadcast() {
    if (!running()) {
      return;
    }

    m_cache = CAST_MOD(Impl)->get_output();

    if (m_update_callback)
      m_update_callback();
    else
      m_log.warn("%s: No handler, ignoring broadcast...", name());
  }

  template <typename Impl>
  void module<Impl>::idle() {
    CAST_MOD(Impl)->sleep(25ms);
  }

  template <typename Impl>
  void module<Impl>::sleep(chrono::duration<double> sleep_duration) {
    std::unique_lock<std::mutex> lck(m_sleeplock);
    m_sleephandler.wait_for(lck, sleep_duration);
  }

  template <typename Impl>
  void module<Impl>::wakeup() {
    m_log.trace("%s: Release sleep lock", name());
    m_sleephandler.notify_all();
  }

  template <typename Impl>
  string module<Impl>::get_format() const {
    return DEFAULT_FORMAT;
  }

  template <typename Impl>
  string module<Impl>::get_output() {
    if (!running()) {
      m_log.trace("%s: Module is disabled", name());
      return "";
    }

    auto format_name = CONST_MOD(Impl).get_format();
    auto format = m_formatter->get(format_name);

    int i = 0;
    bool tag_built = true;

    for (auto&& tag : string_util::split(format->value, ' ')) {
      bool is_blankspace = tag.empty();

      if (tag[0] == '<' && tag[tag.length() - 1] == '>') {
        if (i > 0)
          m_builder->space(format->spacing);
        if (!(tag_built = CONST_MOD(Impl).build(m_builder.get(), tag)) && i > 0)
          m_builder->remove_trailing_space(format->spacing);
        if (tag_built)
          i++;
      } else if (is_blankspace && tag_built) {
        m_builder->node(" ");
      } else if (!is_blankspace) {
        m_builder->node(tag);
      }
    }

    return format->decorate(m_builder.get(), m_builder->flush());
  }

  // }}}
}

POLYBAR_NS_END

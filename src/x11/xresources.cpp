#include <X11/Xlib.h>

#include <utility>

#include "utils/factory.hpp"
#include "x11/xlib.hpp"
#include "x11/xresources.hpp"

POLYBAR_NS

/**
 * Configure injection module
 */
di::injector<const xresource_manager&> configure_xresource_manager() {
  auto instance = factory_util::generic_singleton<xresource_manager>();
  return di::make_injector(di::bind<>().to(instance));
}

/**
 * Construct manager instance
 */
xresource_manager::xresource_manager() {
  XrmInitialize();

  if (xlib::get_display() == nullptr) {
    return;
  }
  if ((m_manager = XResourceManagerString(xlib::get_display())) == nullptr) {
    return;
  }
  if ((m_db = XrmGetStringDatabase(m_manager)) == nullptr) {
    return;
  }
}

/**
 * Get string value from the X resource db
 */
string xresource_manager::get_string(string name, string fallback) const {
  auto result = load_value(move(name), "String", 64);
  if (result.empty()) {
    return fallback;
  }
  return result;
}

/**
 * Get float value from the X resource db
 */
float xresource_manager::get_float(string name, float fallback) const {
  auto result = load_value(move(name), "String", 64);
  if (result.empty()) {
    return fallback;
  }
  return strtof(result.c_str(), nullptr);
}

/**
 * Get integer value from the X resource db
 */
int xresource_manager::get_int(string name, int fallback) const {
  auto result = load_value(move(name), "String", 64);
  if (result.empty()) {
    return fallback;
  }
  return atoi(result.c_str());
}

/**
 * Query the database for given key
 */
string xresource_manager::load_value(const string& key, const string& res_type, size_t n) const {
  if (m_manager == nullptr) {
    return "";
  }
  char* type = nullptr;
  XrmValue ret;
  XrmGetResource(m_db, key.c_str(), res_type.c_str(), &type, &ret);

  if (ret.addr != nullptr && !std::strncmp(res_type.c_str(), type, n)) {
    return {ret.addr};
  }

  return "";
}

POLYBAR_NS_END

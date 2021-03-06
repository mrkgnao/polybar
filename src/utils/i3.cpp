#include <xcb/xcb.h>
#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "config.hpp"
#include "utils/i3.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/icccm.hpp"

POLYBAR_NS

namespace i3_util {
  /**
   * Get all i3 root windows
   */
  vector<xcb_window_t> root_windows(connection& conn, const string& output_name) {
    vector<xcb_window_t> roots;
    auto children = conn.query_tree(conn.screen()->root).children();

    for (auto it = children.begin(); it != children.end(); it++) {
      auto wm_name = icccm_util::get_wm_name(conn, *it);
      if (wm_name.compare("[i3 con] output " + output_name) == 0) {
        roots.emplace_back(*it);
      }
    }

    return roots;
  }

  /**
   * Restack given window above the i3 root window
   * defined for the given monitor
   *
   * Fixes the issue with always-on-top window's
   */
  bool restack_above_root(connection& conn, const monitor_t& mon, const xcb_window_t win) {
    for (auto&& root : root_windows(conn, mon->name)) {
      const uint32_t value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
      const uint32_t value_list[2]{root, XCB_STACK_MODE_ABOVE};
      conn.configure_window_checked(win, value_mask, value_list);
      return true;
    }

    return false;
  }
}

POLYBAR_NS_END

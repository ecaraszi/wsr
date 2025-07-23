#pragma once

template <uint max_> struct message_queue_t {
  static constexpr uint message_chars = 24;
  char storage[max_ * message_chars];
  uint rotate_storage = 0;
  queue_t<char *, max_> queue;

  print_head_t head;

  static constexpr uint entry_items_size = max_ + /* "full" */ 1;
  state_t entry_items[entry_items_size];

  static constexpr uint menu_items_size =
      entry_items_size + /* the null terminator */ 1;
  state_t *menu_items[menu_items_size] = {0};

  void clear() {
    queue.clear();
    rotate_storage = 0;
  }

  print_head_t &add() {
    head.start(storage + rotate_storage, message_chars - /* null */ 1);
    head.i_time(clock.minutes).s(F(" "));

    queue.push(storage + rotate_storage);

    rotate_storage += message_chars;
    if (rotate_storage >= sizeof(storage)) {
      rotate_storage = 0;
    }

    uint i = 0;
    for (; i < queue.size(); i++) {
      menu_items[i] = entry_items + i;
      menu_items[i]->name = smart_string_pointer_t(queue[-1 - i]);
      menu_items[i]->state = state_t::ITEM_ONLY;
    }

    if (queue.full()) {
      menu_items[i] = entry_items + i;
      menu_items[i]->name = F("... (queue full)");
      menu_items[i]->state = state_t::ITEM_ONLY;
      i++;
    }

    for (; i < menu_items_size; i++) {
      menu_items[i] = 0;
    }

    return head;
  }
};

message_queue_t<15> logs;
menu_t logs_menu(F("Logs"), logs.menu_items);

print_head_t &log() { return logs.add(); }

message_queue_t<15> errors;
menu_t errors_menu(F("Errors"), errors.menu_items);

void error(smart_string_pointer_t err) { errors.add().s(err).end(); }

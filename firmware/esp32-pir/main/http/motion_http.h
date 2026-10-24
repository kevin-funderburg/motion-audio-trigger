#pragma once

#include <stdbool.h>

void motion_http_start(void);
// Returns true when a notification is queued, not when playback completes.
bool motion_http_notify(void);

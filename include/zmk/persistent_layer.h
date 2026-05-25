/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keymap.h>
#include <zmk/keys.h>

int pdf_switch_and_save(zmk_keymap_layer_id_t layer);
zmk_keymap_layer_id_t pdf_get_persistent_layer(void);

/*
 * Copyright (c) 2024 ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/keymap.h>
#include <zmk/keys.h>

int pdf_settings_save(zmk_keymap_layer_id_t layer);
int pdf_settings_load(void);

zmk_keymap_layer_id_t pdf_get_persistent_layer(void);

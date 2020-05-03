#pragma once
/* Copyright 2020 "Leo" Dmitry Kuznetsov
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software distributed
   under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
   language governing permissions and limitations under the License.
*/
#include "c.h"

begin_c

typedef struct droid_key_text_string_s {
    char text[128];
} droid_key_text_string_t;

droid_key_text_string_t droid_keys_text(int flags, int ch);
int droid_keys_translate(int32_t key_code, int32_t meta_state, int32_t flags);

end_c
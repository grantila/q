/*
 * Copyright 2016 Gustaf Räntilä
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBQ_RX_OBSERVABLE_HPP
#define LIBQ_RX_OBSERVABLE_HPP

// Standard libraries requried by observable operators
#include <map>
#include <unordered_map>

#include <q/functional.hpp>
#include <q/promise.hpp>
#include <q/channel.hpp>
#include <q/options.hpp>
#include <q/concurrency.hpp>
#include <q/timer.hpp>

#include <q-rx/types.hpp>
#include <q-rx/core/core.hpp>

#include <q-rx/observable/core.hpp>
#include <q-rx/observable/observable_readable.hpp>
#include <q-rx/observable/observable.hpp>

#include <q-rx/observable/observable_generic_perform.hpp>

#include <q-rx/observable/creators/create.hpp>
#include <q-rx/observable/creators/empty.hpp>
#include <q-rx/observable/creators/never.hpp>
#include <q-rx/observable/creators/error.hpp>
#include <q-rx/observable/creators/just.hpp>
#include <q-rx/observable/creators/range.hpp>
#include <q-rx/observable/creators/repeat.hpp>
#include <q-rx/observable/creators/start.hpp>
#include <q-rx/observable/creators/from.hpp>
#include <q-rx/observable/creators/with.hpp>

#include <q-rx/observable/transformers/buffer.hpp>
#include <q-rx/observable/transformers/group_by.hpp>
#include <q-rx/observable/transformers/map.hpp>

#include <q-rx/observable/consumers/consume.hpp>

#endif // LIBQ_RX_OBSERVABLE_HPP

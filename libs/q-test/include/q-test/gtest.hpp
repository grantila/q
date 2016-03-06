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

#ifndef LIBQ_TEST_GTEST_HPP
#define LIBQ_TEST_GTEST_HPP

#include <q/pp.hpp>

#ifdef LIBQ_ON_GCC
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wsign-compare"
#endif // LIBQ_ON_GCC

#include <gtest/gtest.h>

#ifdef LIBQ_ON_GCC
#	pragma GCC diagnostic pop
#endif // LIBQ_ON_GCC

#endif // LIBQ_TEST_GTEST_HPP

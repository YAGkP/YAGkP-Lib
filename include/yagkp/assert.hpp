#pragma once

#include <cstdio>
#include <cstdlib>

// YAGKP_ASSERT(condition, message): checks a condition that must always hold,
// in builds without NDEBUG (debug). If it does not hold, prints the
// condition, the message and the place, and aborts. Release and profile
// builds (-DNDEBUG) do not evaluate the condition at all
#ifdef NDEBUG
#define YAGKP_ASSERT(condition, message) ((void)0)
#else
#define YAGKP_ASSERT(condition, message)                                                           \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			std::fprintf(                                                                          \
			    stderr,                                                                            \
			    "Assertion failed in %s\n"                                                         \
			    "  at:        %s:%d\n"                                                             \
			    "  condition: %s\n"                                                                \
			    "  message:   %s\n",                                                               \
			    __func__,                                                                          \
			    __FILE__,                                                                          \
			    __LINE__,                                                                          \
			    #condition,                                                                        \
			    message                                                                            \
			);                                                                                     \
			std::abort();                                                                          \
		}                                                                                          \
	} while (false)
#endif

// YAGKP_STATIC_ASSERT(condition, message): checks a condition at compile time, in every build.
// The same as static_assert. Variadic, so a condition may contain commas, e.g.
// YAGKP_STATIC_ASSERT(std::is_same_v<A, B>, "message")
#define YAGKP_STATIC_ASSERT(...) static_assert(__VA_ARGS__)

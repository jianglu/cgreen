#ifndef MOCKS_HEADER
#define MOCKS_HEADER

#ifdef __cplusplus
  extern "C" {
#endif

#if defined WINCE || defined WIN32
#include <crtdefs.h>
#else
#include <inttypes.h>
#endif
#include <stdarg.h>
#include <cgreen/reporter.h>
#include <cgreen/constraint.h>

#define mock(...) mock_(__func__, #__VA_ARGS__, (intptr_t)__VA_ARGS__ +0)
#define expect(f, ...) expect_(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)
#define always_expect(f, ...) always_expect_(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)
#define expect_never(f) expect_never_(#f, __FILE__, __LINE__)
#define will_return(f, r) will_return_(#f, (intptr_t)r)
#define always_return(f, r) always_return_(#f, (intptr_t)r)
#define will_respond(f, r, ...) will_return_(#f, (intptr_t)r); expect_(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)
#define always_respond(f, r, ...) always_return_(#f, (intptr_t)r); always_expect_(#f, __FILE__, __LINE__, (Constraint *)__VA_ARGS__ +0, (Constraint *)0)

#define mock_enabled() mock_enabled_(__func__)
#define disable_mock(f) disable_mock_(#f)
#define enable_mock(f) enable_mock_(#f)

intptr_t mock_(const char *function, const char *parameters, ...);
void expect_(const char *function, const char *test_file, int test_line, ...);
void always_expect_(const char *function, const char *test_file, int test_line, ...);
void expect_never_(const char *function, const char *test_file, int test_line);
void will_return_(const char *function, intptr_t result);
void always_return_(const char *function, intptr_t result);
void clear_mocks();
void tally_mocks(TestReporter *reporter);

int mock_enabled_(const char *function);
void disable_mock_(const char *function);
void enable_mock_(const char *function);
void disable_all_mocks();

#ifdef __cplusplus
    }
#endif

#endif

#ifndef CONSTRAINT_HEADER
#define CONSTRAINT_HEADER

#ifdef __cplusplus
  extern "C" {
#endif

#if defined WINCE || defined WIN32
#include <crtdefs.h>
#else
#include <inttypes.h>
#endif

#include <inttypes.h>
#include <cgreen/reporter.h>

#define want(parameter, x) want_(#parameter, (intptr_t)x)
#define want_not(parameter, x) want_not_(#parameter, (intptr_t)x)
#define want_string(parameter, x) want_string_(#parameter, x)
#define want_double(parameter, x) want_double_(#parameter, box_double(x))
#define want_non_null(parameter) want_non_null_(#parameter)

#define set(parameter, x) set_(#parameter, (intptr_t)x)
#define fill(parameter, x) fill_(#parameter, (intptr_t)x, ((x) ? sizeof( *(x) ) : 0))

#define compare_constraint(c, x) (*c->compare)(c, (intptr_t)x)
#define d(x) box_double(x)

typedef enum {
	CG_CONSTRAINT_WANT,
	CG_CONSTRAINT_WANT_NOT,
	CG_CONSTRAINT_SET,
	CG_CONSTRAINT_FILL,
} CgreenConstraintType;

typedef union {
    double d;
} BoxedDouble;

typedef struct Constraint_ Constraint;
typedef int (*CompareConstraintFunc)(Constraint *, intptr_t);
typedef void (*TestConstraintFunc)(Constraint *, const char *, intptr_t, const char *, int, TestReporter *);
struct Constraint_ {
    const char *parameter;
    void (*destroy)(Constraint *);
    CompareConstraintFunc compare;
    TestConstraintFunc test;
    intptr_t expected;
    intptr_t out_value;
	int copy_size;
	CgreenConstraintType constraint_type;
};

void destroy_constraint(void *constraint);
int is_constraint_parameter(Constraint *constraint, const char *label);
void test_constraint(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter);
Constraint *want_(const char *parameter, intptr_t expected);
Constraint *want_non_null_(const char *parameter);
Constraint *want_not_(const char *parameter, intptr_t expected);
Constraint *want_string_(const char *parameter, char *expected);
Constraint *want_double_(const char *parameter, intptr_t expected);
Constraint *set_(const char *parameter, intptr_t out_value);
Constraint *fill_(const char *parameter, intptr_t out_value, int size);
intptr_t box_double(double d);

#ifdef __cplusplus
    }
#endif

#endif

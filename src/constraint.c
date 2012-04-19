#include <cgreen/constraint.h>
#include <cgreen/assertions.h>

#if defined WINCE || defined WIN32
#include <crtdefs.h>
#else
#include <inttypes.h>
#endif

#include <stdlib.h>
#include <string.h>

static void destroy_empty_constraint(Constraint *constraint);
static int compare_want(Constraint *constraint, intptr_t comparison);
static int compare_want_not(Constraint *constraint, intptr_t comparison);
static void test_want(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter);
static void test_want_not(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter);

static int compare_non_null(Constraint *constraint, intptr_t comparison);
static void test_non_null(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter);

static int compare_want_string(Constraint *constraint, intptr_t comparison);
static void test_want_string(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter);

static int compare_want_double(Constraint *constraint, intptr_t comparison);
static void test_want_double(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter);
static Constraint *create_constraint(const char *parameter);
static void destroy_double_constraint(Constraint *constraint);
static double unbox_double(intptr_t box);
static double as_double(intptr_t box);

void destroy_constraint(void *abstract) {
    Constraint *constraint = (Constraint *)abstract;
    (*constraint->destroy)(constraint);
}

int is_constraint_parameter(Constraint *constraint, const char *parameter) {
    return strcmp(constraint->parameter, parameter) == 0;
}

void test_constraint(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter) {
    (*constraint->test)(constraint, function, actual, test_file, test_line, reporter);
}

Constraint *create_want_constraint(const char *parameter, intptr_t expected, CompareConstraintFunc compare, TestConstraintFunc test, CgreenConstraintType type) {
    Constraint *constraint = create_constraint(parameter);
    constraint->parameter = parameter;
    constraint->compare = compare;
    constraint->test = test;
    constraint->expected = expected;
	constraint->out_value = 0;
	constraint->copy_size = 0;
	constraint->constraint_type = type;
    return constraint;
}

Constraint *want_(const char *parameter, intptr_t expected) {
    return create_want_constraint(parameter, expected, &compare_want, &test_want, CG_CONSTRAINT_WANT);
}

Constraint *want_not_(const char *parameter, intptr_t expected) {
    return create_want_constraint(parameter, expected, &compare_want_not, &test_want_not, CG_CONSTRAINT_WANT_NOT);
}

Constraint *want_non_null_(const char *parameter) {
    return create_want_constraint(parameter, 0, &compare_non_null, &test_non_null, CG_CONSTRAINT_WANT_NOT);
}

Constraint *want_string_(const char *parameter, char *expected) {
    Constraint *constraint = create_constraint(parameter);
    constraint->parameter = parameter;
    constraint->compare = &compare_want_string;
    constraint->test = &test_want_string;
    constraint->expected = (intptr_t)expected;
	constraint->out_value = 0;
	constraint->copy_size = 0;
	constraint->constraint_type = CG_CONSTRAINT_WANT;
    return constraint;
}

static int compare_non_null(Constraint *constraint, intptr_t comparison) {
    return (NULL != comparison);
}

static void test_non_null(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter) {
    (*reporter->assert_true)(
            reporter,
            test_file,
            test_line,
            (*constraint->compare)(constraint, actual),
            "Wanted non-null, but got null in function [%s] parameter [%s]",
            function,
            constraint->parameter);
}

Constraint *want_double_(const char *parameter, intptr_t expected) {
    Constraint *constraint = (Constraint *)malloc(sizeof(Constraint));
    constraint->parameter = parameter;
    constraint->destroy = &destroy_double_constraint;
    constraint->parameter = parameter;
    constraint->compare = &compare_want_double;
    constraint->test = &test_want_double;
    constraint->expected = expected;
	constraint->out_value = 0;
	constraint->copy_size = 0;
	constraint->constraint_type = CG_CONSTRAINT_WANT;
    return constraint;
}


Constraint *set_(const char *parameter, intptr_t out_value) {
	Constraint *constraint = create_constraint(parameter);
	constraint->parameter = parameter;
	constraint->compare = NULL;
	constraint->test = NULL;
	constraint->expected = 0;
	constraint->out_value = out_value;
	constraint->copy_size = 0;
	constraint->constraint_type = CG_CONSTRAINT_SET;
	return constraint;
}

Constraint *fill_(const char *parameter, intptr_t out_value, int size)
{
	Constraint *constraint = create_constraint(parameter);
	constraint->parameter = parameter;
	constraint->compare = NULL;
	constraint->test = NULL;
	constraint->expected = 0;
	constraint->out_value = out_value;
	constraint->copy_size = size;
	constraint->constraint_type = CG_CONSTRAINT_FILL;
	return constraint;
}

intptr_t box_double(double d) {
    BoxedDouble *box = (BoxedDouble *) malloc(sizeof(BoxedDouble));
    box->d = d;
    return (intptr_t)box;
}

static void destroy_empty_constraint(Constraint *constraint) {
    free(constraint);
}

static int compare_want(Constraint *constraint, intptr_t comparison) {
    return (constraint->expected == comparison);
}

static int compare_want_not(Constraint *constraint, intptr_t comparison) {
    return (constraint->expected != comparison);
}

static void test_want(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter) {
    (*reporter->assert_true)(
            reporter,
            test_file,
            test_line,
            (*constraint->compare)(constraint, actual),
            "Wanted [%d], but got [%d] in function [%s] parameter [%s]",
            constraint->expected,
            actual,
            function,
            constraint->parameter);
}

static void test_want_not(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter) {
    (*reporter->assert_true)(
            reporter,
            test_file,
            test_line,
            (*constraint->compare)(constraint, actual),
            "Wanted anything but [%d] in function [%s] parameter [%s]",
            constraint->expected,
            actual,
            function,
            constraint->parameter);
}

static int compare_want_string(Constraint *constraint, intptr_t comparison) {
    return strings_are_equal((const char *)constraint->expected, (const char *)comparison);
}

static void test_want_string(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter) {
    (*reporter->assert_true)(
            reporter,
            test_file,
            test_line,
            (*constraint->compare)(constraint, actual),
            "Wanted [%s], but got [%s] in function [%s] parameter [%s]",
            show_null_as_the_string_null((const char *)constraint->expected),
            show_null_as_the_string_null((const char *)actual),
            function,
            constraint->parameter);
}

static int compare_want_double(Constraint *constraint, intptr_t comparison) {
    return doubles_are_equal(as_double(constraint->expected), as_double(comparison));
}

static void test_want_double(Constraint *constraint, const char *function, intptr_t actual, const char *test_file, int test_line, TestReporter *reporter) {
    (*reporter->assert_true)(
            reporter,
            test_file,
            test_line,
            (*constraint->compare)(constraint, actual),
            "Wanted [%d], but got [%d] in function [%s] parameter [%s]",
            as_double(constraint->expected),
            as_double(actual),
            function,
            constraint->parameter);
    unbox_double(actual);
}

static Constraint *create_constraint(const char *parameter) {
    Constraint *constraint = (Constraint *)malloc(sizeof(Constraint));
    constraint->parameter = parameter;
    constraint->destroy = &destroy_empty_constraint;
    return constraint;
}

static void destroy_double_constraint(Constraint *constraint) {
    unbox_double(constraint->expected);
    destroy_empty_constraint(constraint);
}

static double unbox_double(intptr_t box) {
    double d = as_double(box);
    free((BoxedDouble *)box);
    return d;
}

static double as_double(intptr_t box) {
    return ((BoxedDouble *)box)->d;
}

/* vim: set ts=4 sw=4 et cindent: */

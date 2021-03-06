#include <cgreen/mocks.h>
#include <cgreen/constraint.h>
#include <cgreen/reporter.h>
#include <cgreen/vector.h>
#include <cgreen/parameters.h>
#include <stdlib.h>
#include <string.h>

typedef struct RecordedResult_ {
    const char *function;
    intptr_t result;
    int should_keep;
} RecordedResult;

typedef struct RecordedExpectation_ {
    const char *function;
    const char *test_file;
    int test_line;
    int should_keep;
    CgreenVector *constraints;
} RecordedExpectation;

typedef struct UnwantedCall_ {
    const char *test_file;
    int test_line;
    const char *function;
} UnwantedCall;

static CgreenVector *result_queue = NULL;
static CgreenVector *expectation_queue = NULL;
static CgreenVector *unwanted_calls = NULL;
static CgreenVector *disabled_mocks = NULL;
static CgreenVector *enabled_mocks = NULL;
static char all_mocks_disabled = 0;

intptr_t stubbed_result(const char *function);
static RecordedResult *create_recorded_result(const char *function, intptr_t result);
static void ensure_result_queue_exists();
static RecordedExpectation *create_recorded_expectation(const char *function, const char *test_file, int test_line, va_list constraints);
static void destroy_expectation(void *expectation);
static void ensure_expectation_queue_exists();
static void ensure_unwanted_calls_list_exists();
RecordedResult *find_result(const char *function);
static void unwanted_check(const char *function);
void trigger_unfulfilled_expectations(CgreenVector *expectation_queue, TestReporter *reporter);
RecordedExpectation *find_expectation(const char *function);
void apply_any_constraints(RecordedExpectation *expectation, const char *parameter, intptr_t actual);

intptr_t mock_(const char *function, const char *parameters, ...) {
    RecordedExpectation *expectation = NULL;
    unwanted_check(function);
    expectation = find_expectation(function);
    if (expectation != NULL) {
        CgreenVector *names = create_vector_of_names(parameters);
        int i;
        va_list actual;
        va_start(actual, parameters);
        for (i = 0; i < cgreen_vector_size(names); i++) {
            apply_any_constraints(expectation, (const char *)cgreen_vector_get(names, i), va_arg(actual, intptr_t));
        }
        va_end(actual);
        destroy_cgreen_vector(names);
        if (! expectation->should_keep) {
            destroy_expectation(expectation);
        }
    }
    return stubbed_result(function);
}

void expect_(const char *function, const char *test_file, int test_line, ...) {
    va_list constraints;
    RecordedExpectation *expectation = NULL;
    va_start(constraints, test_line);
    expectation = create_recorded_expectation(function, test_file, test_line, constraints);
    va_end(constraints);
    expectation->should_keep = 0;
}

void always_expect_(const char *function, const char *test_file, int test_line, ...) {
    RecordedExpectation *expectation = NULL;
    va_list constraints;
    va_start(constraints, test_line);
    expectation = create_recorded_expectation(function, test_file, test_line, constraints);
    va_end(constraints);
    expectation->should_keep = 1;
}

void expect_never_(const char *function, const char *test_file, int test_line) {
    UnwantedCall *unwanted = NULL;
    ensure_unwanted_calls_list_exists();
    unwanted = (UnwantedCall *)malloc(sizeof(UnwantedCall));
    unwanted->test_file = test_file;
    unwanted->test_line = test_line;
    unwanted->function = function;
    cgreen_vector_add(unwanted_calls, unwanted);
}

void will_return_(const char *function, intptr_t result) {
    RecordedResult *record = create_recorded_result(function, result);
    record->should_keep = 0;
}

void always_return_(const char *function, intptr_t result) {
    RecordedResult *record = create_recorded_result(function, result);
    record->should_keep = 1;
}

void clear_mocks() {
    if (result_queue != NULL) {
        destroy_cgreen_vector(result_queue);
		result_queue = NULL;
    }
    if (expectation_queue != NULL) {
        destroy_cgreen_vector(expectation_queue);
		expectation_queue = NULL;
    }
    if (unwanted_calls != NULL) {
        destroy_cgreen_vector(unwanted_calls);
		unwanted_calls = NULL;
    }
    if (disabled_mocks != NULL) {
        destroy_cgreen_vector(disabled_mocks);
		disabled_mocks = NULL;
    }
    if (enabled_mocks != NULL) {
        destroy_cgreen_vector(enabled_mocks);
		enabled_mocks = NULL;
    }
    all_mocks_disabled = 0;
}

void tally_mocks(TestReporter *reporter) {
    trigger_unfulfilled_expectations(expectation_queue, reporter);
    clear_mocks();
}

intptr_t stubbed_result(const char *function) {
    intptr_t value = 0;
    RecordedResult *result = find_result(function);
    if (result == NULL) {
        return 0;
    }
    value = result->result;
    if (! result->should_keep) {
        free(result);
    }
    return value;
}

static RecordedResult *create_recorded_result(const char *function, intptr_t result) {
    RecordedResult *record = NULL;

    ensure_result_queue_exists();
    record = (RecordedResult *)malloc(sizeof(RecordedResult));
    record->function = function;
    record->result = result;
    cgreen_vector_add(result_queue, record);
    return record;
}

static void ensure_result_queue_exists() {
    if (result_queue == NULL) {
        result_queue = create_cgreen_vector(&free);
    }
}

static RecordedExpectation *create_recorded_expectation(const char *function, const char *test_file, int test_line, va_list constraints) {
    RecordedExpectation *expectation = NULL;
    Constraint *constraint = NULL;

    ensure_expectation_queue_exists();
    expectation = (RecordedExpectation *)malloc(sizeof(RecordedExpectation));
    expectation->function = function;
    expectation->test_file = test_file;
    expectation->test_line = test_line;
    expectation->constraints = create_cgreen_vector(&destroy_constraint);

    while ((constraint = va_arg(constraints, Constraint *)) != (Constraint *)0) {
        cgreen_vector_add(expectation->constraints, constraint);
    }
    cgreen_vector_add(expectation_queue, expectation);
    return expectation;
}

static void destroy_expectation(void *abstract) {
    RecordedExpectation *expectation = (RecordedExpectation *)abstract;
    destroy_cgreen_vector(expectation->constraints);
    free(expectation);
}

static void ensure_expectation_queue_exists() {
    if (expectation_queue == NULL) {
        expectation_queue = create_cgreen_vector(&destroy_expectation);
    }
}

static void ensure_unwanted_calls_list_exists() {
    if (unwanted_calls == NULL) {
        unwanted_calls = create_cgreen_vector(&free);
    }
}

RecordedResult *find_result(const char *function) {
    int i;
    for (i = 0; i < cgreen_vector_size(result_queue); i++) {
        RecordedResult *result = (RecordedResult *)cgreen_vector_get(result_queue, i);
        if (strcmp(result->function, function) == 0) {
            if (! result->should_keep) {
                return (RecordedResult *) cgreen_vector_remove(result_queue, i);
            }
            return result;
        }
    }
    return NULL;
}

static void unwanted_check(const char *function) {
    int i;
    for (i = 0; i < cgreen_vector_size(unwanted_calls); i++) {
        UnwantedCall *unwanted = (UnwantedCall *) cgreen_vector_get(unwanted_calls, i);
        if (strcmp(unwanted->function, function) == 0) {
            (*get_test_reporter()->assert_true)(
                    get_test_reporter(),
                    unwanted->test_file,
                    unwanted->test_line,
                    0,
                    "Unexpected call to function [%s]", function);
        }
    }
}

void trigger_unfulfilled_expectations(CgreenVector *expect_queue,
                                      TestReporter *reporter) {
    int i;
    for (i = 0; i < cgreen_vector_size(expectation_queue); i++) {
        RecordedExpectation *expectation = cgreen_vector_get(expect_queue, i);
        if (! expectation->should_keep) {
            (*reporter->assert_true)(
                    reporter,
                    expectation->test_file,
                    expectation->test_line,
                    0,
                    "Call was not made to function [%s]", expectation->function);
        }
    }
}

RecordedExpectation *find_expectation(const char *function) {
    int i;
    for (i = 0; i < cgreen_vector_size(expectation_queue); i++) {
        RecordedExpectation *expectation =
                (RecordedExpectation *)cgreen_vector_get(expectation_queue, i);
        if (strcmp(expectation->function, function) == 0) {
            if (! expectation->should_keep) {
                return (RecordedExpectation *) cgreen_vector_remove(expectation_queue, i);
            }
            return expectation;
        }
    }
    return NULL;
}

void apply_any_constraints(RecordedExpectation *expectation, const char *parameter, intptr_t actual) {
    int i;
    for (i = 0; i < cgreen_vector_size(expectation->constraints); i++) {
        Constraint *constraint = (Constraint *)cgreen_vector_get(expectation->constraints, i);
        if (is_constraint_parameter(constraint, parameter)) {
			switch(constraint->constraint_type)
			{
			case CG_CONSTRAINT_WANT:
				test_constraint(
						constraint,
						expectation->function,
						actual,
						expectation->test_file,
						expectation->test_line,
						get_test_reporter());
			break;
			case CG_CONSTRAINT_SET:
				*((int *)actual) = (int)constraint->out_value;
				break;

			case CG_CONSTRAINT_FILL:
				memcpy((void *)actual, (void *)constraint->out_value, constraint->copy_size);
				break;
			}
        }
    }
}

void disable_all_mocks() {
    all_mocks_disabled = 1;
}

int mock_enabled_(const char *function) {
    int i;
    if (all_mocks_disabled) {
        // Mocks disabled by default, check for any that are enabled
        for (i = 0; i < cgreen_vector_size(enabled_mocks); i++) {
            char *enabled_mock = (char *)cgreen_vector_get(enabled_mocks, i);
            if (strcmp(enabled_mock, function) == 0) {
                return 1;
            }
        }
        return 0;
    } else {
        // Mocks enabled by default, check for any that are disabled
        for (i = 0; i < cgreen_vector_size(disabled_mocks); i++) {
            char *disabled_mock = (char *)cgreen_vector_get(disabled_mocks, i);
            if (strcmp(disabled_mock, function) == 0) {
                return 0;
            }
        }
        return 1;
    }
    // Should never reach here
    return 1;
}

void disable_mock_(const char *function) {
    if (disabled_mocks == NULL) {
        disabled_mocks = create_cgreen_vector(NULL);
    }
    cgreen_vector_add(disabled_mocks, (void *)function);
}

void enable_mock_(const char *function) {
    if (enabled_mocks == NULL) {
        enabled_mocks = create_cgreen_vector(NULL);
    }
    cgreen_vector_add(enabled_mocks, (void *)function);
}

/* vim: set ts=4 sw=4 et cindent: */

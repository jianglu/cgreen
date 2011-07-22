#include <cgreen/text_reporter.h>
#include <cgreen/reporter.h>
#include <cgreen/breadcrumb.h>
#include <stdlib.h>
#include <stdio.h>

static void text_reporter_start_suite(TestReporter *reporter, const char *name, const int number_of_tests);
static void text_reporter_start_test(TestReporter *reporter, const char *name);
static void text_reporter_finish(TestReporter *reporter, const char *name);
static void show_fail(TestReporter *reporter, const char *file, int line, const char *message, va_list arguments);
static void show_incomplete(TestReporter *reporter, const char *name);
static void show_breadcrumb(const char *name, void *memo);

TestReporter *create_text_reporter(void) {
    TestReporter *reporter = create_reporter();
    if (reporter == NULL) {
        return NULL;
    }
    reporter->start_suite = &text_reporter_start_suite;
    reporter->start_test = &text_reporter_start_test;
    reporter->show_fail = &show_fail;
    reporter->show_incomplete = &show_incomplete;
#ifdef CG_FILE_LOG
    reporter->fOutput = fopen("cg_results.txt", "wt");
#endif
    reporter->finish_test = &text_reporter_finish;
    reporter->finish_suite = &text_reporter_finish;
    return reporter;
}

static void text_reporter_start_suite(TestReporter *reporter, const char *name, const int number_of_tests) {
	reporter_start(reporter, name);
	if (get_breadcrumb_depth((CgreenBreadcrumb *)reporter->breadcrumb) == reporter->log_depth) {
#ifdef CG_FILE_LOG
        fprintf(reporter->fOutput, "Running \"%s\"...\n", get_current_from_breadcrumb((CgreenBreadcrumb *)reporter->breadcrumb));
#endif
		printf("Running \"%s\"...\n",
		       get_current_from_breadcrumb((CgreenBreadcrumb *)reporter->breadcrumb));
	}
}

static void text_reporter_start_test(TestReporter *reporter, const char *name) {
	reporter_start(reporter, name);
}

static void text_reporter_finish(TestReporter *reporter, const char *name) {
	reporter_finish(reporter, name);
	if (get_breadcrumb_depth((CgreenBreadcrumb *)reporter->breadcrumb) == 0) {
#ifdef CG_FILE_LOG
        fprintf(reporter->fOutput,
				"Completed \"%s\": %d pass%s, %d failure%s, %d exception%s.\n",
				name,
				reporter->passes,
				reporter->passes == 1 ? "" : "es",
				reporter->failures,
				reporter->failures == 1 ? "" : "s",
				reporter->exceptions,
				reporter->exceptions == 1 ? "" : "s");
		fclose(reporter->fOutput);
#endif
		printf(
				"Completed \"%s\": %d pass%s, %d failure%s, %d exception%s.\n",
				name,
				reporter->passes,
				reporter->passes == 1 ? "" : "es",
				reporter->failures,
				reporter->failures == 1 ? "" : "s",
				reporter->exceptions,
				reporter->exceptions == 1 ? "" : "s");
	}
}

static void show_fail(TestReporter *reporter, const char *file, int line, const char *message, va_list arguments) {
    int i = 0;
#ifdef CG_FILE_LOG
    fprintf(reporter->fOutput, "%s:%d: unit test failure: ", file, line);
#endif
    printf("%s:%d: unit test failure: ", file, line);
    walk_breadcrumb(
            (CgreenBreadcrumb *)reporter->breadcrumb,
            &show_breadcrumb,
            (void *)&i);
#ifdef CG_FILE_LOG
    vfprintf(reporter->fOutput, (message == NULL ? "Problem" : message), arguments);
    fprintf(reporter->fOutput, " at [%s] line [%d]\n\n", file, line);
#endif
    vprintf((message == NULL ? "Problem" : message), arguments);
    printf(" at [%s] line [%d]\n", file, line);
}

static void show_incomplete(TestReporter *reporter, const char *name) {
    int i = 0;
#ifdef CG_FILE_LOG
    fprintf(reporter->fOutput, "Exception!: ");
#endif
    printf("Exception!: ");
    walk_breadcrumb(
            (CgreenBreadcrumb *)reporter->breadcrumb,
            &show_breadcrumb,
            (void *)&i);
#ifdef CG_FILE_LOG
    fprintf(reporter->fOutput, "Test \"%s\" failed to complete\n\n", name);
#endif
    printf("Test \"%s\" failed to complete\n", name);
}

static void show_breadcrumb(const char *name, void *memo) {
    if (*(int *)memo > 0) {
        printf("%s -> ", name);
    }
    (*(int *)memo)++;
}

/* vim: set ts=4 sw=4 et cindent: */

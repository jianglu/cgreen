#include <cgreen/unit.h>
#include <cgreen/reporter.h>
#include <cgreen/mocks.h>
#include <cgreen/parameters.h>
#include <cgreen/assertions.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined WINCE

#include <windows.h>

#elif defined WIN32

#include <windows.h>
#include <signal.h>

#elif defined IPHONE

#include <pthread.h>
#include <objc/runtime.h>
#include <objc/message.h>

#else

#include <unistd.h>
#include <sys/wait.h>

#endif

#if defined(WINCE) || defined(WIN32) || defined(IPHONE)
typedef void (*sighandler_t)(int);
#else
#include <signal.h>
#endif

#if defined WINCE || defined WIN32
#define strdup _strdup
#endif


enum {test_function, test_suite};

typedef struct {
    int type;
    union {
        void (*test)();
        TestSuite *suite;
    } sPtr;
    char *name;
} UnitTest;

struct TestSuite_ {
	const char *name;
    UnitTest *tests;
    void (*setup)();
    void (*teardown)();
    int size;
};

#if defined WIN32 || defined IPHONE
typedef struct
{
    TestSuite *suite;
    UnitTest *test;
    TestReporter *reporter;
} CgTestParams;
#endif

static void clean_up_test_run(TestSuite *suite, TestReporter *reporter);
static void run_every_test(TestSuite *suite, TestReporter *reporter);
static void run_named_test(TestSuite *suite, char *name, TestReporter *reporter);
static int has_test(TestSuite *suite, char *name);
static void run_test_in_the_current_process(TestSuite *suite, UnitTest *test, TestReporter *reporter);
static void run_test_in_its_own_process(TestSuite *suite, UnitTest *test, TestReporter *reporter);

#ifndef WIN32
static int in_child_process();
static void wait_for_child_process();
#endif

static void ignore_ctrl_c();
static void allow_ctrl_c();
static void stop();
static void run_the_test_code(TestSuite *suite, UnitTest *test, TestReporter *reporter);
static void tally_counter(const char *file, int line, int expected, int actual, void *abstract_reporter);
static void die(const char *message, ...);
static void do_nothing();

TestSuite *create_named_test_suite(const char *name) {
    TestSuite *suite = (TestSuite *)malloc(sizeof(TestSuite));
	suite->name = name;
    suite->tests = NULL;
    suite->setup = &do_nothing;
    suite->teardown = &do_nothing;
    suite->size = 0;
    return suite;
}

void destroy_test_suite(TestSuite *suiteToDestroy) {
	int i;
	for (i = 0; i < suiteToDestroy->size; i++) {
		UnitTest test = suiteToDestroy->tests[i];
		TestSuite* suite = test.sPtr.suite;
		if (test_suite == test.type && suite != NULL) {
			suiteToDestroy->tests[i].sPtr.suite = NULL;
            destroy_test_suite(suite);
		}
	}
    if (suiteToDestroy->tests != NULL)
		free(suiteToDestroy->tests);

    free(suiteToDestroy);
}

void add_test_(TestSuite *suite, char *name, CgreenTest *test) {
    suite->size++;
    suite->tests = (UnitTest *)realloc(suite->tests, sizeof(UnitTest) * suite->size);
    suite->tests[suite->size - 1].type = test_function;
    suite->tests[suite->size - 1].name = strdup(name);
    suite->tests[suite->size - 1].sPtr.test = test;
}

void add_tests_(TestSuite *suite, const char *names, ...) {
    CgreenVector *test_names = create_vector_of_names(names);
    int i;
    va_list tests;
    va_start(tests, names);
    for (i = 0; i < cgreen_vector_size(test_names); i++) {
        add_test_(suite, (char *)cgreen_vector_get(test_names, i), va_arg(tests, CgreenTest *));
    }
    va_end(tests);
    destroy_cgreen_vector(test_names);
}

void add_suite_(TestSuite *owner, char *name, TestSuite *suite) {
    owner->size++;
    owner->tests = (UnitTest *)realloc(owner->tests, sizeof(UnitTest) * owner->size);
    owner->tests[owner->size - 1].type = test_suite;
    owner->tests[owner->size - 1].name = name;
    owner->tests[owner->size - 1].sPtr.suite = suite;
}

void setup_(TestSuite *suite, void (*setup)()) {
    suite->setup = setup;
}

void teardown_(TestSuite *suite, void (*teardown)()) {
    suite->teardown = teardown;
}

#if !defined(WIN32) && !defined(IPHONE)
void die_in(unsigned int seconds) {
    signal(SIGALRM, (sighandler_t)&stop);
    alarm(seconds);
}
#endif

int count_tests(TestSuite *suite) {
    int count = 0;
    int i;
    for (i = 0; i < suite->size; i++) {
        if (suite->tests[i].type == test_function) {
            count++;
        } else {
            count += count_tests(suite->tests[i].sPtr.suite);
        }
    }
    return count;
}

int run_test_suite(TestSuite *suite, TestReporter *reporter) {
    int success = 0;
    if (reporter == NULL) {
        return EXIT_FAILURE;
    }
    success = setup_reporting(reporter);
    if (success < 0) {
        return EXIT_FAILURE;
    }
    run_every_test(suite, reporter);
    success = (reporter->failures == 0 && reporter->exceptions == 0);
    clean_up_test_run(suite, reporter);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int run_single_test(TestSuite *suite, char *name, TestReporter *reporter) {
    int success = 0;
    if (reporter == NULL) {
        return EXIT_FAILURE;
    }
    success = setup_reporting(reporter);
    if (success < 0) {
        return EXIT_FAILURE;
    }
    run_named_test(suite, name, reporter);
    success = (reporter->failures == 0 && reporter->exceptions == 0);
    clean_up_test_run(suite, reporter);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void clean_up_test_run(TestSuite *suite, TestReporter *reporter) {
    (*reporter->destroy)(reporter);
    destroy_test_suite(suite);
}

static void run_every_test(TestSuite *suite, TestReporter *reporter) {
    int i = 0;

    (*reporter->start_suite)(reporter, suite->name, count_tests(suite));
    for (i = 0; i < suite->size; i++) {
        if (suite->tests[i].type == test_function) {
            run_test_in_its_own_process(suite, &(suite->tests[i]), reporter);
        } else {
            (*suite->setup)();
            run_every_test(suite->tests[i].sPtr.suite, reporter);
            (*suite->teardown)();
        }
    }
    send_reporter_completion_notification(reporter);
    (*reporter->finish_suite)(reporter, suite->name);
}

static void run_named_test(TestSuite *suite, char *name, TestReporter *reporter) {
    int i = 0;

    (*reporter->start_suite)(reporter, suite->name, count_tests(suite));
    for (i = 0; i < suite->size; i++) {
        if (suite->tests[i].type == test_function) {
            if (strcmp(suite->tests[i].name, name) == 0) {
                run_test_in_the_current_process(suite, &(suite->tests[i]), reporter);
            }
        } else if (has_test(suite->tests[i].sPtr.suite, name)) {
            (*suite->setup)();
            run_named_test(suite->tests[i].sPtr.suite, name, reporter);
            (*suite->teardown)();
        }
    }
    send_reporter_completion_notification(reporter);
    (*reporter->finish_suite)(reporter, suite->name);
}

static int has_test(TestSuite *suite, char *name) {
	int i;
	for (i = 0; i < suite->size; i++) {
        if (suite->tests[i].type == test_function) {
            if (strcmp(suite->tests[i].name, name) == 0) {
                return 1;
            }
        } else if (has_test(suite->tests[i].sPtr.suite, name)) {
            return 1;
        }
	}
	return 0;
}

static void run_test_in_the_current_process(TestSuite *suite, UnitTest *test, TestReporter *reporter) {
    (*reporter->start_test)(reporter, test->name);
    run_the_test_code(suite, test, reporter);
    send_reporter_completion_notification(reporter);
    (*reporter->finish_test)(reporter, test->name);
}


#ifdef WIN32
unsigned int run_test_thread(void* pVoid)
{
    CgTestParams* pTestParams = (CgTestParams*)pVoid;
    if(!pTestParams)
        return (unsigned int)-1;

    __try
    {
        run_the_test_code(pTestParams->suite, pTestParams->test, pTestParams->reporter);
    }
    __finally
    {
        send_reporter_completion_notification(pTestParams->reporter);
    }

    free(pTestParams);
    return 0;
}
#elif defined IPHONE
unsigned int iphone_test_thread(void *pVoid){
    CgTestParams* pTestParams = (CgTestParams*)pVoid;

    void *pool = objc_msgSend(objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_getUid("alloc")), sel_getUid("init"));

    if(!pTestParams)
        return (unsigned int)-1;

    run_the_test_code(pTestParams->suite, pTestParams->test, pTestParams->reporter);
    send_reporter_completion_notification(pTestParams->reporter);

    free(pTestParams);
	objc_msgSend(pool, sel_getUid("release"));
    return 0;
}
#endif

// Hacked to run tests in their own thread, not their own process.
static void run_test_in_its_own_process(TestSuite *suite, UnitTest *test, TestReporter *reporter) {
#if defined WIN32
    HANDLE pHandle = NULL;
#elif defined IPHONE
	pthread_t thread;
	pthread_attr_t attr;
#endif

#if defined WIN32 || defined IPHONE
    CgTestParams* pThreadParams = malloc(sizeof(CgTestParams));
    if(!pThreadParams)
        return;
    pThreadParams->reporter = reporter;
    pThreadParams->suite = suite;
    pThreadParams->test = test;
#endif

    (*reporter->start_test)(reporter, test->name);

#if defined WIN32
    pHandle = (VOID *)CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) &run_test_thread,
                                   (LPVOID) pThreadParams,
                                   0,
                                   NULL);
    WaitForSingleObject(pHandle, INFINITE);
    (*reporter->finish_test)(reporter, test->name);
#elif defined IPHONE
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&thread, &attr, (void *)iphone_test_thread, (void *)pThreadParams);
	pthread_attr_destroy(&attr);
    pthread_join(thread, NULL);
    (*reporter->finish_test)(reporter, test->name);
#else
    if (in_child_process()) {
        run_the_test_code(suite, test, reporter);
        send_reporter_completion_notification(reporter);
        stop();
    } else {
        wait_for_child_process();
        (*reporter->finish_test)(reporter, test->name);
    }
#endif
}

#if !defined(WIN32) && !defined(IPHONE)
static int in_child_process() {
    pid_t child = fork();
    if (child < 0) {
        die("Could not fork process\n");
    }
    return ! child;
}
#endif

#ifndef WIN32
static void wait_for_child_process() {
    int status;
    ignore_ctrl_c();
    wait(&status);
    allow_ctrl_c();
}

static void ignore_ctrl_c() {
    signal(SIGINT, SIG_IGN);
}

static void allow_ctrl_c() {
    signal(SIGINT, SIG_DFL);
}

static void stop() {
    exit(EXIT_SUCCESS);
}
#endif

static void run_the_test_code(TestSuite *suite, UnitTest *test, TestReporter *reporter) {
    significant_figures_for_assert_double_are(8);
    clear_mocks();
    (*suite->setup)();
    (*test->sPtr.test)();
    (*suite->teardown)();
    tally_mocks(reporter);
}

static void tally_counter(const char *file, int line, int expected, int actual, void *abstract_reporter) {
    TestReporter *reporter = (TestReporter *)abstract_reporter;
    (*reporter->assert_true)(
            reporter,
            file,
            line,
            (actual == expected),
            "Expected a call count of [%d], but got [%d]",
            expected,
            actual);
}

static void die(const char *message, ...) {
	va_list arguments;
	va_start(arguments, message);
	vprintf(message, arguments);
	va_end(arguments);
	exit(EXIT_FAILURE);
}

static void do_nothing() {
}

/* vim: set ts=4 sw=4 et cindent: */

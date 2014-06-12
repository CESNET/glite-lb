#include <cstdlib>
#include <cstring>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include <sys/time.h>
#include "context.h"


class ContextTest: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ContextTest);
	CPPUNIT_TEST(testContextReinit);
	CPPUNIT_TEST(testContextMulti);
	CPPUNIT_TEST(testParams);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() {
	}


	void tearDown() {
	}


	void testContextReinit() {
		edg_wll_Context ctx;
		size_t i;

		for (i = 0; i < 10; i++) {
			CPPUNIT_ASSERT(edg_wll_InitContext(&ctx) == 0);
			edg_wll_FreeContext(ctx);
		}
	}


	void testContextMulti() {
		edg_wll_Context ctxs[100];
		size_t i;

		for (i = 0; i < sizeof(ctxs)/sizeof(ctxs[0]); i++) {
			CPPUNIT_ASSERT(edg_wll_InitContext(&(ctxs[i])) == 0);
		}
		for (i = 0; i < sizeof(ctxs)/sizeof(ctxs[0]); i++) {
			edg_wll_FreeContext(ctxs[i]);
		}
	}


	void testParams() {
		edg_wll_Context ctx;
		edg_wll_ContextParam param;
		int i, i2;
		char *s, *s2;
		struct timeval t, t2;

		CPPUNIT_ASSERT(edg_wll_InitContext(&ctx) == 0);

		for (param = EDG_WLL_PARAM_HOST; param < EDG_WLL_PARAM__LAST; param = (edg_wll_ContextParam)(param + 1)) {
			std::cout << param << " ";
			switch (param) {
				case EDG_WLL_PARAM_LEVEL:
				case EDG_WLL_PARAM_DESTINATION_PORT:
				case EDG_WLL_PARAM_QUERY_SERVER_PORT:
				case EDG_WLL_PARAM_NOTIF_SERVER_PORT:
				case EDG_WLL_PARAM_QUERY_JOBS_LIMIT:
				case EDG_WLL_PARAM_QUERY_EVENTS_LIMIT:
				case EDG_WLL_PARAM_QUERY_RESULTS:
				case EDG_WLL_PARAM_CONNPOOL_SIZE:
				case EDG_WLL_PARAM_SOURCE:
					// 0 is the default - set on-zero
					i = 1 + rand() * 10 / RAND_MAX;
					i2 = 1;
					CPPUNIT_ASSERT(edg_wll_SetParamInt(ctx, param, i) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &i2) == 0);
					CPPUNIT_ASSERT(i == i2);

					// 0 is the default - set on-zero
					i = 1 + rand() * 10 / RAND_MAX;
					i2 = 1;
					CPPUNIT_ASSERT(edg_wll_SetParam(ctx, param, i) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &i2) == 0);
					CPPUNIT_ASSERT(i == i2);

					break;

				case EDG_WLL_PARAM_HOST:
				case EDG_WLL_PARAM_INSTANCE:
				case EDG_WLL_PARAM_DESTINATION:
				case EDG_WLL_PARAM_QUERY_SERVER:
				case EDG_WLL_PARAM_NOTIF_SERVER:
				case EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE:
				case EDG_WLL_PARAM_X509_PROXY:
				case EDG_WLL_PARAM_X509_KEY:
				case EDG_WLL_PARAM_X509_CERT:
				case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:
				case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:
				case EDG_WLL_PARAM_LBPROXY_USER:
				case EDG_WLL_PARAM_JPREG_TMPDIR:
			        case EDG_WLL_PARAM_LOG_FILE_PREFIX:
			        case EDG_WLL_PARAM_LOG_IL_SOCK:
				case EDG_WLL_PARAM_LBPROXY_SERVERNAME:
					s = (char *)"nothing";
					CPPUNIT_ASSERT(edg_wll_SetParamString(ctx, param, NULL) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &s2) == 0);
					// s2 is default value (may be NULL)
					CPPUNIT_ASSERT(s != s2);
					free(s2);

					s = (char *)"nothing";
					CPPUNIT_ASSERT(edg_wll_SetParam(ctx, param, NULL) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &s2) == 0);
					// s2 is default value (may be NULL)
					CPPUNIT_ASSERT(s != s2);
					free(s2);

					randstr(&s);
					CPPUNIT_ASSERT(edg_wll_SetParamString(ctx, param, s) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &s2) == 0);
					if (
						param != EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE &&
						param != EDG_WLL_PARAM_LBPROXY_SERVERNAME
					) CPPUNIT_ASSERT(strcmp(s, s2) == 0);
					free(s);
					free(s2);

					randstr(&s);
					CPPUNIT_ASSERT(edg_wll_SetParam(ctx, param, s) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &s2) == 0);
					if (
						param != EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE &&
						param != EDG_WLL_PARAM_LBPROXY_SERVERNAME
					) CPPUNIT_ASSERT(strcmp(s, s2) == 0);
					free(s);
					free(s2);

					break;

				case EDG_WLL_PARAM_LOG_TIMEOUT:
				case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT:
				case EDG_WLL_PARAM_QUERY_TIMEOUT:
				case EDG_WLL_PARAM_NOTIF_TIMEOUT:
					randtime(&t);
					t2.tv_sec = 1;
					t2.tv_usec = 1;
					CPPUNIT_ASSERT(edg_wll_SetParamTime(ctx, param, &t) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &t2) == 0);
					CPPUNIT_ASSERT(t.tv_sec == t2.tv_sec);
					CPPUNIT_ASSERT(t.tv_usec == t2.tv_usec);

					randtime(&t);
					t2.tv_sec = 1;
					t2.tv_usec = 1;
					CPPUNIT_ASSERT(edg_wll_SetParam(ctx, param, &t) == 0);
					CPPUNIT_ASSERT(edg_wll_GetParam(ctx, param, &t2) == 0);
					CPPUNIT_ASSERT(t.tv_sec == t2.tv_sec);
					CPPUNIT_ASSERT(t.tv_usec == t2.tv_usec);

					break;

				default:
					CPPUNIT_ASSERT("Unknown (or new?) parameter" && false);
					break;
			}
		}

		edg_wll_FreeContext(ctx);
	}

private:
	void randstr(char **s) {
		size_t len, i;

		len = (double)rand() / RAND_MAX * 100;
		*s = (char *)malloc(len + 1);
		for (i = 0; i < len; i++) (*s)[i] = (double)rand() / RAND_MAX * 255;
		(*s)[len] = '\0';
	}

	void randtime(struct timeval *t) {
		if (rand() < RAND_MAX / 2) {
			t->tv_sec = (double)rand() * 2 * 365 * 24 *3600 / RAND_MAX;
		} else {
			t->tv_sec = (double)rand() * 10 / RAND_MAX;
		}
		t->tv_usec = (double)rand() * 1000000.0 / RAND_MAX;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(ContextTest);

#include "test_main.cpp"

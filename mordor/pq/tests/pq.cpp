// Copyright (c) 2009 - Mozy, Inc.

#include "mordor/predef.h"

#include <iostream>

#include <boost/date_time/posix_time/posix_time_io.hpp>

#include "mordor/config.h"
#include "mordor/iomanager.h"
#include "mordor/main.h"
#include "mordor/pq/connection.h"
#include "mordor/pq/exception.h"
#include "mordor/pq/transaction.h"
#include "mordor/version.h"
#include "mordor/statistics.h"
#include "mordor/streams/memory.h"
#include "mordor/streams/transfer.h"
#include "mordor/test/antxmllistener.h"
#include "mordor/test/test.h"
#include "mordor/test/stdoutlistener.h"

using namespace Mordor;
using namespace Mordor::PQ;
using namespace Mordor::Test;

static ConfigVar<std::string>::ptr g_xmlDirectory = Config::lookup<std::string>(
    "test.antxml.directory", std::string(), "Location to put XML files");

std::string g_goodConnString;
std::string g_badConnString;

MORDOR_MAIN(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
            << " <connection string>"
            << std::endl;
        return 1;
    }
    g_goodConnString = argv[1];
    --argc;
    ++argv;
    Config::loadFromEnvironment();

    std::shared_ptr<TestListener> listener;
    std::string xmlDirectory = g_xmlDirectory->val();
    if (!xmlDirectory.empty()) {
        if (xmlDirectory == ".")
            xmlDirectory.clear();
        listener.reset(new AntXMLListener(xmlDirectory));
    } else {
        listener.reset(new StdoutListener());
    }
    bool result;
    if (argc > 1) {
        result = runTests(testsForArguments(argc - 1, argv + 1), *listener);
    } else {
        result = runTests(*listener);
    }
    std::cout << Statistics::dump();
    return result ? 0 : 1;
}

#ifdef WINDOWS
#define MORDOR_PQ_UNITTEST(TestName)                                            \
    static void PQ_ ## TestName(IOManager *ioManager);                          \
    MORDOR_UNITTEST(PQ, TestName)                                               \
    {                                                                           \
        PQ_ ## TestName(NULL);                                                  \
    }                                                                           \
    static void PQ_ ## TestName(IOManager *ioManager)
#else
#define MORDOR_PQ_UNITTEST(TestName)                                            \
    static void PQ_ ## TestName(IOManager *ioManager);                          \
    MORDOR_UNITTEST(PQ, TestName ## Blocking)                                   \
    {                                                                           \
        PQ_ ## TestName(NULL);                                                  \
    }                                                                           \
    MORDOR_UNITTEST(PQ, TestName ## Async)                                      \
    {                                                                           \
                                                           \
        IOManager ioManager;                                                    \
        PQ_ ## TestName(&ioManager);                                            \
    }                                                                           \
    static void PQ_ ## TestName(IOManager *ioManager)
#endif

void constantQuery(const std::string &queryName = std::string(),
    IOManager *ioManager = NULL)
{
    Connection conn(g_goodConnString, ioManager);
    PreparedStatement stmt = conn.prepare("SELECT 1, 'mordor'", queryName);
    Result result = stmt.execute();
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 2u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<int>(0u, (size_t)0u), 1);
    MORDOR_TEST_ASSERT_EQUAL(result.get<long long>(0u, (size_t)0u), 1);
    MORDOR_TEST_ASSERT_EQUAL(result.get<const char *>(0u, 1u), "mordor");
    MORDOR_TEST_ASSERT_EQUAL(result.get<std::string>(0u, 1u), "mordor");
}

MORDOR_PQ_UNITTEST(constantQuery)
{ constantQuery(std::string(), ioManager); }
MORDOR_PQ_UNITTEST(constantQueryPrepared)
{ constantQuery("constant", ioManager); }

MORDOR_PQ_UNITTEST(invalidConnString)
{
    MORDOR_TEST_ASSERT_EXCEPTION(Connection conn("garbage", ioManager),
        ConnectionException);
}

MORDOR_PQ_UNITTEST(invalidConnString2)
{
    MORDOR_TEST_ASSERT_EXCEPTION(Connection conn("garbage=", ioManager), ConnectionException);
}

MORDOR_PQ_UNITTEST(invalidConnString3)
{
    MORDOR_TEST_ASSERT_EXCEPTION(Connection conn("host=garbage", ioManager), ConnectionException);
}

MORDOR_PQ_UNITTEST(badConnString)
{
    MORDOR_TEST_ASSERT_EXCEPTION(Connection conn(g_badConnString, ioManager), ConnectionException);
}

#ifndef WINDOWS
#define closesocket close
#endif

void queryAfterDisconnect(IOManager *ioManager = NULL)
{
    Connection conn(g_goodConnString, ioManager);

    closesocket(PQsocket(conn.conn()));
    MORDOR_TEST_ASSERT_EXCEPTION(conn.execute("SELECT 1"), ConnectionException);
    conn.reset();
    Result result = conn.execute("SELECT 1");
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<int>(0u, (size_t)0u), 1);
}

MORDOR_PQ_UNITTEST(queryAfterDisconnect)
{ queryAfterDisconnect(ioManager); }

void fillUsers(Connection &conn)
{
    conn.execute("CREATE TEMPORARY TABLE users (id INTEGER, name TEXT, height SMALLINT, awesome BOOLEAN, company TEXT, gender CHAR, efficiency REAL, crazy DOUBLE PRECISION, sometime TIMESTAMP, version INTEGER[4])");
    conn.execute("INSERT INTO users VALUES (1, 'cody', 72, true, 'Mozy', 'M', .9, .75, '2009-05-19 15:53:45.123456', '{1,3,5,7}')");
    conn.execute("INSERT INTO users VALUES (2, 'brian', 70, false, NULL, 'M', .9, .25, NULL, '{}')");
}

template <class ParamType, class ExpectedType>
void queryForParam(const std::string &query, ParamType param, size_t expectedCount,
    ExpectedType expected,
    const std::string &queryName = std::string(), IOManager *ioManager = NULL)
{
    Connection conn(g_goodConnString, ioManager);
    fillUsers(conn);
    PreparedStatement stmt = conn.prepare(query, queryName);
    Result result = stmt.execute(param);
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), expectedCount);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<ExpectedType>(0u, (size_t)0u), expected);
}

MORDOR_PQ_UNITTEST(queryForInt)
{ queryForParam("SELECT name FROM users WHERE id=$1", 2, 1u, "brian", std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForIntPrepared)
{ queryForParam("SELECT name FROM users WHERE id=$1::integer", 2, 1u, "brian", "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForString)
{ queryForParam("SELECT id FROM users WHERE name=$1", "brian", 1u, 2, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForStringPrepared)
{ queryForParam("SELECT id FROM users WHERE name=$1::text", "brian", 1u, 2, "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForSmallInt)
{ queryForParam("SELECT id FROM users WHERE height=$1", (short)70, 1u, 2, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForSmallIntPrepared)
{ queryForParam("SELECT id FROM users WHERE height=$1::smallint", (short)70, 1u, 2, "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForBoolean)
{ queryForParam("SELECT id FROM users WHERE awesome=$1", false, 1u, 2, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForBooleanPrepared)
{ queryForParam("SELECT id FROM users WHERE awesome=$1::boolean", false, 1u, 2, "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForChar)
{ queryForParam("SELECT id FROM users WHERE gender=$1", 'M', 2u, 1, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForCharPrepared)
{ queryForParam("SELECT id FROM users WHERE gender=$1::CHAR", 'M', 2u, 1, "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForFloat)
{ queryForParam("SELECT efficiency FROM users WHERE efficiency=$1", .9f, 2u, .9f, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForFloatPrepared)
{ queryForParam("SELECT efficiency FROM users WHERE efficiency=$1::REAL", .9f, 2u, .9f, "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForDouble)
{ queryForParam("SELECT crazy FROM users WHERE crazy=$1", .75, 1u, .75, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForDoublePrepared)
{ queryForParam("SELECT crazy FROM users WHERE crazy=$1::DOUBLE PRECISION", .75, 1u, .75, "constant", ioManager); }

MORDOR_PQ_UNITTEST(queryForIntArray)
{
    std::vector<int> expected(4);
    expected[0] = 1; expected[1] = 3; expected[2] = 5; expected[3] = 7;
    queryForParam("SELECT version FROM users WHERE id=$1", 1, 1u, expected, std::string(), ioManager);
}
MORDOR_PQ_UNITTEST(queryForIntArrayPrepared)
{
    std::vector<int> expected(4);
    expected[0] = 1; expected[1] = 3; expected[2] = 5; expected[3] = 7;
    queryForParam("SELECT version FROM users WHERE id=$1::INTEGER", 1, 1u, expected, "constant", ioManager);
}

MORDOR_PQ_UNITTEST(queryForEmptyIntArray)
{ queryForParam("SELECT version FROM users WHERE id=$1", 2, 1u, std::vector<int>(), std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForEmptyIntArrayPrepared)
{ queryForParam("SELECT version FROM users WHERE id=$1::INTEGER", 2, 1u, std::vector<int>(), "constant", ioManager); }

static const boost::posix_time::ptime thetime(
    boost::gregorian::date(2009, 05, 19),
    boost::posix_time::hours(15) + boost::posix_time::minutes(53) +
    boost::posix_time::seconds(45) + boost::posix_time::microseconds(123456));

static const boost::posix_time::ptime nulltime;

MORDOR_PQ_UNITTEST(queryForTimestamp)
{ queryForParam("SELECT sometime FROM users WHERE sometime=$1", thetime, 1u, thetime, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForTimestampPrepared)
{ queryForParam("SELECT sometime FROM users WHERE sometime=$1::TIMESTAMP", thetime, 1u, thetime, "constant", ioManager); }
MORDOR_PQ_UNITTEST(queryForNullTimestamp)
{ queryForParam("SELECT sometime FROM users WHERE sometime IS NULL OR sometime=$1", nulltime, 1u, nulltime, std::string(), ioManager); }
MORDOR_PQ_UNITTEST(queryForNullTimestampPrepared)
{ queryForParam("SELECT sometime FROM users WHERE sometime IS NULL OR sometime=$1", nulltime, 1u, nulltime, "constant", ioManager); }

MORDOR_UNITTEST(PQ, transactionCommits)
{
    Connection conn(g_goodConnString);
    fillUsers(conn);
    Transaction t(conn);
    conn.execute("UPDATE users SET name='tom' WHERE id=1");
    t.commit();
    Result result = conn.execute("SELECT name FROM users WHERE id=1");
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<const char *>(0u, (size_t)0u), "tom");
}

MORDOR_UNITTEST(PQ, transactionRollsback)
{
    Connection conn(g_goodConnString);
    fillUsers(conn);
    Transaction t(conn);
    conn.execute("UPDATE users SET name='tom' WHERE id=1");
    t.rollback();
    Result result = conn.execute("SELECT name FROM users WHERE id=1");
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<const char *>(0u, (size_t)0u), "cody");
}

MORDOR_UNITTEST(PQ, transactionRollsbackAutomatically)
{
    Connection conn(g_goodConnString);
    fillUsers(conn);
    {
        Transaction t(conn);
        conn.execute("UPDATE users SET name='tom' WHERE id=1");
    }
    Result result = conn.execute("SELECT name FROM users WHERE id=1");
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<const char *>(0u, (size_t)0u), "cody");
}

static void copyIn(IOManager *ioManager = NULL)
{
    Connection conn(g_goodConnString, ioManager);
    conn.execute("CREATE TEMP TABLE stuff (id INTEGER, name TEXT)");
    Stream::ptr stream = conn.copyIn("stuff").csv()();
    stream->write("1,cody\n");
    stream->write("2,tom\n");
    stream->write("3,brian\n");
    stream->write("4,jeremy\n");
    stream->write("5,zach\n");
    stream->write("6,paul\n");
    stream->write("7,alen\n");
    stream->write("8,jt\n");
    stream->write("9,jon\n");
    stream->write("10,jacob\n");
    stream->close();
    Result result = conn.execute("SELECT COUNT(*) FROM stuff");
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<long long>(0u, (size_t)0u), 10);
    result = conn.execute("SELECT SUM(id) FROM stuff");
    MORDOR_TEST_ASSERT_EQUAL(result.rows(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.columns(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(result.get<long long>(0u, (size_t)0u), 55);
}

MORDOR_PQ_UNITTEST(copyIn)
{ copyIn(ioManager); }

static void copyOut(IOManager *ioManager = NULL)
{
    Connection conn(g_goodConnString, ioManager);
    conn.execute("CREATE TEMP TABLE country (code TEXT, name TEXT)");
    PreparedStatement stmt = conn.prepare("INSERT INTO country VALUES($1, $2)",
        "insertcountry");
    Transaction transaction(conn);
    stmt.execute("AF", "AFGHANISTAN");
    stmt.execute("AL", "ALBANIA");
    stmt.execute("DZ", "ALGERIA");
    stmt.execute("ZM", "ZAMBIA");
    stmt.execute("ZW", "ZIMBABWE");

    Stream::ptr stream = conn.copyOut("country").csv().delimiter('|')();
    MemoryStream output;
    transferStream(stream, output);
    MORDOR_TEST_ASSERT(output.buffer() ==
        "AF|AFGHANISTAN\n"
        "AL|ALBANIA\n"
        "DZ|ALGERIA\n"
        "ZM|ZAMBIA\n"
        "ZW|ZIMBABWE\n");
}

MORDOR_PQ_UNITTEST(copyOut)
{ copyOut(ioManager); }

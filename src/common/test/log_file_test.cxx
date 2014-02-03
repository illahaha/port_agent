/*******************************************************************************
 * Filename: log_file_test.cxx
 * Author: Bill French (wfrench@ucsd.edu)
 * License: Apache 2.0
 *
 * Test all of the log_file class methods.
 ******************************************************************************/

#include "common/exception.h"
#include "common/log_file.h"
#include "common/logger.h"
#include "common/util.h"
#include "gmock/gmock.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

using namespace std;
using namespace logger;


#define LOGFILE "/tmp/gtest_logger.log"
#define LOGBASE "/tmp/gtest_logger"
#define LOGEXT  "log"

class LogFileTest : public testing::Test {
    
    protected:
        virtual void SetUp() {
            Logger::SetLogFile("/tmp/gtest.log");
            Logger::SetLogLevel("MESG");
        	LOG(INFO) << "Start Log File Test";
        	remove_file(LOGFILE);
        }
    
        virtual void TearDown() {
			remove_file(LOGFILE);
            remove_file(Logger::Instance()->getLogFilename().c_str());
        }
    
        ~LogFileTest() {
        }

        string getDate() {
            // Build a string to be what we think the logfile should be
            char buffer[11];
            time_t t;
            time(&t);
            tm r = {0};
            strftime(buffer, sizeof(buffer), "%Y%m%d", localtime_r(&t, &r));

            return string(buffer);
        }
		
		string paddedNumber(int num) {
			char buffer[3];
			sprintf(buffer, "%02d", num);
			return buffer;
		}
};

/* Test Construction and Option setting */
TEST_F(LogFileTest, CtorTest) {
	LOG(DEBUG) << "There";

    // Set the log file and check it was set correctly
    LogFile logfile;
	LOG(DEBUG) << "here";
	logfile.setFile(LOGFILE);
	LOG(DEBUG) << "here";
    EXPECT_EQ(logfile.getFilename(), LOGFILE);

    // Set the logbase, but the log file should still be LOGFILE
    // because it takes precidence
    logfile.setBase(LOGBASE);
    EXPECT_EQ(logfile.getFilename(), LOGFILE);

	LOG(DEBUG) << "There";
    // Check that logbase works now, first with an extension.
    LogFile newfile;
    newfile.setBase(LOGBASE, LOGEXT);
    
    ostringstream expected;
    expected << LOGBASE << "." << getDate() << "." << LOGEXT;

    EXPECT_EQ(expected.str(), newfile.getFilename());
	LOG(DEBUG) << "There";

    // Now lets check without an extension
    LogFile finalfile;
    finalfile.setBase(LOGBASE);

    ostringstream expected2;
    expected2 << LOGBASE << "." << getDate();
	LOG(DEBUG) << "There";

    EXPECT_EQ(expected2.str(), finalfile.getFilename());
}

// Test log write
TEST_F(LogFileTest, LogOutputTest) {
	LogFile log(LOGFILE);
	string msg = "test message";
	string result;
    ofstream *out = log.getStreamObject();

    EXPECT_EQ(log.getFilename(), LOGFILE);

    LOG(DEBUG) << "Write '" << msg << "' to " << log.getFilename();

    EXPECT_TRUE(out->good());
	EXPECT_TRUE(*out << msg);
    EXPECT_TRUE(out->good());
    log.close();

	result = read_file(log.getFilename().c_str());
	EXPECT_EQ(result, msg);
}


// test stream insertion operator
TEST_F(LogFileTest, LogInsertionOperator) {
	LogFile log(LOGFILE);
	string msg = "test message";
	string result;

	log << msg;
	log.close();

	result = read_file(LOGFILE);
	EXPECT_EQ(result, msg);
}


// test log write on a closed file handle
TEST_F(LogFileTest, ClosedFileTest) {
	LogFile log(LOGFILE);
	string result;

	log << "test message\n";
	log.close();

	result = read_file(LOGFILE);
	EXPECT_EQ(result, "test message\n");

	log << "second message\n";
	log.close();

	result = read_file(LOGFILE);
	LOG(DEBUG) << "Contents of " << LOGFILE << " '" << result << "'";
	EXPECT_THAT(result, testing::MatchesRegex("test message.*second message.*"));
}    

// test log write when file removed
TEST_F(LogFileTest, RemovedFileTest) {
	LogFile log(LOGFILE);
	string result;

	log << "test message" << endl;
	log.close();

	result = read_file(LOGFILE);
	EXPECT_EQ(result, "test message\n");
	remove_file(LOGFILE);

	log << "removed file";
    log.close();
	result = read_file(LOGFILE);

	LOG(DEBUG) << "Contents of " << LOGFILE << " '" << result << "'";

	EXPECT_EQ(result, "removed file");
}

// test log write when file exists
TEST_F(LogFileTest, FileExistsTest) {
	LogFile log(LOGFILE);
	string result;

	log << "test message\n";
	log.close();

	result = read_file(LOGFILE);
	EXPECT_EQ(result, "test message\n");

	// Now start up a new logger.
	LogFile newlog(LOGFILE);
	newlog << "second message\n";
	newlog.close();

	result = read_file(LOGFILE);
	LOG(DEBUG) << "Contents of " << LOGFILE << " '" << result << "'";
	EXPECT_THAT(result, testing::MatchesRegex("test message.*second message.*"));

}

// test dualing log writes
// Open two loggers to the same file
TEST_F(LogFileTest, DualingLogTest) {
	LogFile loga(LOGFILE);
	LogFile logb(LOGFILE);
    string result;

    // We have to flush after every log write because it's buffered IO
	loga << "a";
    loga.flush();
	logb << "b";
    logb.flush();
	loga << "a";
    loga.flush();
	logb << "b";
    logb.flush();

	loga.close();
    logb.close();

	result = read_file(LOGFILE);
	EXPECT_EQ(result, "abab");
}

// test write method
TEST_F(LogFileTest, WriteLogTest) {
	LogFile log(LOGFILE);
    string result;

	log.write("\n\r", 2);
    log.close();

	result = read_file(LOGFILE);
	EXPECT_EQ(result, "\n\r");
}


// test permission denied
TEST_F(LogFileTest, PermissionDeniedTest) {
	LogFile log("/tmp");
    bool caughtError = false;

    try {
	    // This should blow up!
    	log << "BOOM!";
    }
    catch(OOIException & exception) {
    	string msg = exception.what();

    	caughtError = true;
    	LOG(DEBUG) << "Caught exception: " << msg;
    };

    EXPECT_TRUE(caughtError);
}

// test writing without file or base set
TEST_F(LogFileTest, DerivedLogFileTest) {
	LogFile log;
    bool caughtError = false;

    try {
	    // This should blow up!
    	log << "BOOM!";
    }
    catch(OOIException & exception) {
    	string msg = exception.what();

    	caughtError = true;
    	LOG(DEBUG) << "Caught exception: " << msg;
    };

    EXPECT_TRUE(caughtError);
}

// Test the == operator
TEST_F(LogFileTest, EqualityOperator) {
	LogFile lhsLog, rhsLog;

    EXPECT_TRUE(lhsLog == lhsLog);
    EXPECT_TRUE(lhsLog == rhsLog);
	
	lhsLog.setFile("foo");
	EXPECT_FALSE(lhsLog == rhsLog);
	rhsLog.setFile("foo");
	EXPECT_TRUE(lhsLog == rhsLog);
	
	lhsLog.setBase("foo");
	EXPECT_FALSE(lhsLog == rhsLog);
	rhsLog.setBase("foo");
	EXPECT_TRUE(lhsLog == rhsLog);
	
	lhsLog.setBase("foo", "ext");
	EXPECT_FALSE(lhsLog == rhsLog);
	rhsLog.setBase("foo", "ext");
	EXPECT_TRUE(lhsLog == rhsLog);
}

TEST_F(LogFileTest, LogFileRotationType) {
	LogFile log;
	ostringstream expected;
	char buffer[7];
	time_t ts;
    time (&ts);
    struct tm * timeinfo = localtime(&ts);
	int hour = timeinfo->tm_hour;
	int min = timeinfo->tm_min;
	int sec = timeinfo->tm_sec;
	
	log.setBase("foo", "ext");
	log.setRotation(DAILY);
	
	expected << "foo." << getDate() << ".ext";
	EXPECT_EQ(log.getFilename(), expected.str());
	expected.str("");
	expected.clear();
	
	log.setRotation(HOURLY);
	expected << "foo." << getDate() << "_" << paddedNumber(hour) << "0000" << ".ext";
	EXPECT_EQ(log.getFilename(), expected.str());
	expected.str("");
	expected.clear();
	
	log.setRotation(QUARTER_HOURLY);
	expected << "foo." << getDate() << "_" << paddedNumber(hour) << paddedNumber((min/15)*15) << "00" << ".ext";
	EXPECT_EQ(log.getFilename(), expected.str());
	expected.str("");
	expected.clear();
	
	log.setRotation(MINUTE);
	expected << "foo." << getDate() << "_" << paddedNumber(hour) << paddedNumber(min) << "00" << ".ext";
	EXPECT_EQ(log.getFilename(), expected.str());
	expected.str("");
	expected.clear();
	
	log.setRotation(SECOND);
	expected << "foo." << getDate() << "_" << paddedNumber(hour) << paddedNumber(min) << paddedNumber(sec) << ".ext";
	EXPECT_EQ(log.getFilename(), expected.str());
}

TEST_F(LogFileTest, LogFileRotation) {
	LogFile log;
	string result;
	string file1;
	string file2;
	
	log.setBase(LOGBASE, LOGEXT);
	log.setRotation(SECOND);
	file1 = log.getFilename();
	
	remove_file(file1.c_str());
	log << "foo";
	result = read_file(file1.c_str());
	EXPECT_TRUE(result.length());
	
	// Let some time elapse so we have to roll a file
	sleep(2);
	file2 = log.getFilename();
	EXPECT_NE(file1, file2);
	log << "foo";
	result = read_file(file2.c_str());
	EXPECT_TRUE(result.length());
}



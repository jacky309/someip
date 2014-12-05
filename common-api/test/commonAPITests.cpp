#include "SomeIPRuntime.h"
#include "CommonAPI/Runtime.h"

#include "gtest/gtest.h"

class SomeIP_CommonAPI_Test : public::testing::Test {

protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	SomeIP_CommonAPI_Test() {
		// You can do set-up work for each test here.
	}

	virtual ~SomeIP_CommonAPI_Test() {
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	//	virtual void SetUp() {
	//		// Code here will be called immediately after the constructor (right before each test).
	//		log_error();
	//	}

	//	virtual void TearDown() {
	//		// Code here will be called immediately after each test (right before the destructor).
	//		log_error();
	//	}

};

using namespace CommonAPI;

/**
 * Send a message to ourself, using two distinct connections.
 */
TEST_F(SomeIP_CommonAPI_Test, RegistrationTest) {

	std::shared_ptr<Runtime> defaultRuntime = Runtime::load();
	EXPECT_TRUE(defaultRuntime.get() != nullptr);

    auto runtimeById = Runtime::load(CommonAPI::SomeIP::SomeIPRuntime::ID);
	EXPECT_TRUE(runtimeById.get() != nullptr);

}



int main(int argc, char** argv) {
//	MainLoopApplication app; // to get rid of the DLT threads, by forcing the DLT main loop mode
	::testing::InitGoogleTest(&argc, argv);
	auto ret = RUN_ALL_TESTS();
	return ret;
}


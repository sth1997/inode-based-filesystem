add_test( test_mytest.printA /Users/sth/git/inode-based-filesystem/build/bin/mytest_test [==[--gtest_filter=test_mytest.printA]==] --gtest_also_run_disabled_tests)
set_tests_properties( test_mytest.printA PROPERTIES WORKING_DIRECTORY /Users/sth/git/inode-based-filesystem/build/test)
set( mytest_test_TESTS test_mytest.printA)

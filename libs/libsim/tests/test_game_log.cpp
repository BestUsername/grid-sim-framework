#include "gtest/gtest.h"
#include "libsim/game_log.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace grid::libsim;

TEST(GameLogTest, StartsEmpty) {
    GameLog log;
    EXPECT_EQ(log.size(), 0u);
    EXPECT_TRUE(log.getEntries().empty());
}

TEST(GameLogTest, LogSingleEntry) {
    GameLog log;
    log.log("Alice", "(5, 10)", Senses::Hearing, "Hello world!");

    ASSERT_EQ(log.size(), 1u);
    auto entries = log.getEntries();
    EXPECT_EQ(entries[0].source, "Alice");
    EXPECT_EQ(entries[0].location, "(5, 10)");
    EXPECT_EQ(entries[0].sense, Senses::Hearing);
    EXPECT_EQ(entries[0].message, "Hello world!");
}

TEST(GameLogTest, LogMultipleEntries) {
    GameLog log;
    log.log("Alice", "(0, 0)", Senses::Hearing, "First");
    log.log("Bob",   "(3, 4)", Senses::Sight,   "Second");
    log.log("Carol", "(7, 2)", Senses::Hearing, "Third");

    ASSERT_EQ(log.size(), 3u);
    auto entries = log.getEntries();
    EXPECT_EQ(entries[0].source, "Alice");
    EXPECT_EQ(entries[1].source, "Bob");
    EXPECT_EQ(entries[2].source, "Carol");
}

TEST(GameLogTest, TimestampsAreOrdered) {
    GameLog log;
    log.log("A", "(0, 0)", Senses::Hearing, "First");
    log.log("B", "(1, 1)", Senses::Hearing, "Second");

    auto entries = log.getEntries();
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_LE(entries[0].timestamp, entries[1].timestamp);
}

TEST(GameLogTest, ClearRemovesAllEntries) {
    GameLog log;
    log.log("A", "(0, 0)", Senses::Hearing, "msg");
    log.log("B", "(1, 1)", Senses::Sight,   "msg");
    ASSERT_EQ(log.size(), 2u);

    log.clear();
    EXPECT_EQ(log.size(), 0u);
    EXPECT_TRUE(log.getEntries().empty());
}

TEST(GameLogTest, GetEntriesReturnsACopy) {
    GameLog log;
    log.log("A", "(0, 0)", Senses::Hearing, "before");

    auto snapshot = log.getEntries();
    log.log("B", "(1, 1)", Senses::Hearing, "after");

    // The snapshot should not contain the entry added after the copy
    EXPECT_EQ(snapshot.size(), 1u);
    EXPECT_EQ(log.size(), 2u);
}

TEST(GameLogTest, DifferentSenses) {
    GameLog log;
    log.log("X", "(0, 0)", Senses::Sight,   "I see you");
    log.log("X", "(0, 0)", Senses::Hearing, "I hear you");
    log.log("X", "(0, 0)", Senses::Smell,   "I smell you");
    log.log("X", "(0, 0)", Senses::Touch,   "I feel you");
    log.log("X", "(0, 0)", Senses::Taste,   "I taste you");

    auto entries = log.getEntries();
    ASSERT_EQ(entries.size(), 5u);
    EXPECT_EQ(entries[0].sense, Senses::Sight);
    EXPECT_EQ(entries[1].sense, Senses::Hearing);
    EXPECT_EQ(entries[2].sense, Senses::Smell);
    EXPECT_EQ(entries[3].sense, Senses::Touch);
    EXPECT_EQ(entries[4].sense, Senses::Taste);
}

TEST(GameLogTest, FormatContainsAllFields) {
    LogEntry entry{
        std::chrono::steady_clock::time_point{},  // epoch
        "Alice", "(5, 10)", Senses::Hearing, "Hello!"
    };
    std::string formatted = entry.format();
    EXPECT_NE(formatted.find("Alice"), std::string::npos);
    EXPECT_NE(formatted.find("(5, 10)"), std::string::npos);
    EXPECT_NE(formatted.find("Hearing"), std::string::npos);
    EXPECT_NE(formatted.find("Hello!"), std::string::npos);
    // Should contain the @ separator between source and location
    EXPECT_NE(formatted.find("Alice @ (5, 10)"), std::string::npos);
}

TEST(GameLogTest, WritesToLogFile) {
    const std::string path = "test_game_log_output.log";
    {
        GameLog log;
        log.setLogFile(path);
        log.log("Alice", "(1, 2)", Senses::Hearing, "Hello!");
        log.log("Bob",   "(3, 4)", Senses::Sight,   "I see you");
    } // destructor closes the file

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());

    std::string line1, line2, line3;
    std::getline(in, line1);
    std::getline(in, line2);
    bool has_third = static_cast<bool>(std::getline(in, line3));

    EXPECT_NE(line1.find("Alice"), std::string::npos);
    EXPECT_NE(line1.find("Hello!"), std::string::npos);
    EXPECT_NE(line2.find("Bob"), std::string::npos);
    EXPECT_NE(line2.find("I see you"), std::string::npos);
    EXPECT_FALSE(has_third);  // only 2 lines

    std::remove(path.c_str());
}

TEST(GameLogTest, NoFileByDefault) {
    // Without setLogFile, logging should still work (memory only)
    GameLog log;
    log.log("X", "(0, 0)", Senses::Hearing, "test");
    EXPECT_EQ(log.size(), 1u);
}

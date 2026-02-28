#include <gtest/gtest.h>
#include <stdexcept>
#include "args.h"

TEST(ParseArgs, EmptyArgsReturnsEmptyMap) {
	char* argv[] = { const_cast<char*>("prog") };
	auto args = parse_args(1, argv);
	EXPECT_TRUE(args.empty());
}

TEST(ParseArgs, SingleKeyValue) {
	char* argv[] = {
		const_cast<char*>("prog"),
		const_cast<char*>("--scene=tokamak")
	};
	auto args = parse_args(2, argv);
	EXPECT_EQ(args.size(), 1u);
	EXPECT_EQ(args["scene"], "tokamak");
}

TEST(ParseArgs, MultipleKeyValues) {
	char* argv[] = {
		const_cast<char*>("prog"),
		const_cast<char*>("--scene=free_space"),
		const_cast<char*>("--initialParticles=50000"),
		const_cast<char*>("--width=800")
	};
	auto args = parse_args(4, argv);
	EXPECT_EQ(args.size(), 3u);
	EXPECT_EQ(args["scene"], "free_space");
	EXPECT_EQ(args["initialParticles"], "50000");
	EXPECT_EQ(args["width"], "800");
}

TEST(ParseArgs, RejectsArgWithoutLeadingDashes) {
	char* argv[] = {
		const_cast<char*>("prog"),
		const_cast<char*>("scene=tokamak")
	};
	EXPECT_THROW(parse_args(2, argv), std::invalid_argument);
}

TEST(ParseArgs, RejectsArgWithoutEquals) {
	char* argv[] = {
		const_cast<char*>("prog"),
		const_cast<char*>("--scene")
	};
	EXPECT_THROW(parse_args(2, argv), std::invalid_argument);
}

TEST(ExtractParams, ParsesSceneTypeTokamak) {
	std::unordered_map<std::string, std::string> args = {
		{"scene", "tokamak"}
	};
	auto params = extract_params(args);
	EXPECT_EQ(params.sceneType, SCENE_TYPE_TOKAMAK);
}

TEST(ExtractParams, ParsesSceneTypeFreeSpace) {
	std::unordered_map<std::string, std::string> args = {
		{"scene", "free_space"}
	};
	auto params = extract_params(args);
	EXPECT_EQ(params.sceneType, SCENE_TYPE_FREE_SPACE);
}

TEST(ExtractParams, InvalidSceneTypeThrows) {
	std::unordered_map<std::string, std::string> args = {
		{"scene", "invalid"}
	};
	EXPECT_THROW(extract_params(args), std::invalid_argument);
}

TEST(ExtractParams, ParsesNumericParams) {
	std::unordered_map<std::string, std::string> args = {
		{"scene", "tokamak"},
		{"initialParticles", "1000"},
		{"width", "1920"},
		{"height", "1080"},
		{"fps", "30"}
	};
	auto params = extract_params(args);
	EXPECT_EQ(params.sceneType, SCENE_TYPE_TOKAMAK);
	EXPECT_EQ(params.initialParticles, 1000u);
	EXPECT_EQ(params.windowWidth, 1920u);
	EXPECT_EQ(params.windowHeight, 1080u);
	EXPECT_EQ(params.targetFPS, 30u);
}

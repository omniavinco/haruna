﻿// Ŭnicode please
#include "test_stdafx.h"
#include "sora/low_level_c_file.h"
#include "sora/memory_file.h"

TEST(LowLevelCFile, test) 
{
	using sora::LowLevelCFile;
	const char *path1 = "low_level_c_file.txt";

	LowLevelCFile file1(path1);
	EXPECT_EQ(false, file1.IsOpened());
	ASSERT_EQ(true, file1.Open("rb"));

	EXPECT_EQ(28, file1.GetLength());
	const char *content = "this is line1.this is line2.";
	const char *buffer = (const char *)file1.GetBuffer();
	EXPECT_STREQ(content, buffer);
}

TEST(ReadonlyFile_WriteonlyFile, test) 
{
	using sora::WriteonlyCFile;
	using sora::ReadonlyCFile;
	const char path1[] = "asdf.txt";
	const char content[] = "this is line1.this is line2.";

	WriteonlyCFile file1(path1);
	EXPECT_EQ(false, file1.IsOpened());
	ASSERT_EQ(true, file1.Open());
	file1.Write(content, sizeof(content));
	file1.Close();

	ReadonlyCFile file2(path1);
	EXPECT_EQ(false, file2.IsOpened());
	EXPECT_EQ(true, file2.Open());
	EXPECT_EQ(sizeof(content), file2.GetLength());
	const char *buffer = (const char *)file2.GetBuffer();
	EXPECT_STREQ(content, buffer);
}

TEST(MemoryFile, test) 
{
	using sora::MemoryFile;
	const char *path1 = "low_level_c_file.txt";

	MemoryFile file1(path1);
	EXPECT_EQ(false, file1.IsOpened());
	ASSERT_EQ(true, file1.Open());

	EXPECT_EQ(28, file1.GetLength());
	const char *content = "this is line1.this is line2.";
	EXPECT_STREQ(content, (const char*)file1.start);
}
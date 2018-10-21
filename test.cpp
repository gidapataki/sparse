#include <iostream>
#include "sparse.h"


template<typename U, typename V>
void expect_eq(int line, const U& expected, const V& actual) {
	if (!(expected == actual)) {
		std::cerr << "Error in line " << line << std::endl;
		std::cerr << "  expected: " << expected << std::endl;
		std::cerr << "    actual: " << actual << std::endl;
	}
}

template<typename U, typename V>
void expect_ne(int line, const U& expected, const V& actual) {
	if (!(expected != actual)) {
		std::cerr << "Error in line " << line << std::endl;
		std::cerr << "  expected: " << expected << std::endl;
		std::cerr << "    actual: " << actual << std::endl;
	}
}

#define EXPECT_EQ(u, v) expect_eq(__LINE__, u, v)
#define EXPECT_NE(u, v) expect_ne(__LINE__, u, v)


void test_resize() {
	sparse::Matrix<int> m;

	// size
	EXPECT_EQ(0, m.row_size());
	EXPECT_EQ(0, m.col_size());

	// resize
	m.resize(2, 4);
	EXPECT_EQ(2, m.row_size());
	EXPECT_EQ(4, m.col_size());
	EXPECT_EQ((void*) nullptr, m.find(1, 2));
}


void test_insert() {
	sparse::Matrix<int> m(2, 4);

	// insert
	m.insert(0, 0, 5);
	m.insert(0, 3, 6);
	m.insert(1, 0, 7);
	m.insert(1, 2, 8);
	m.insert(1, 3, 9);

	// find
	EXPECT_EQ((void*) nullptr, m.find(1, 1));

	EXPECT_NE((void*) nullptr, m.find(0, 0));
	EXPECT_NE((void*) nullptr, m.find(0, 3));
	EXPECT_NE((void*) nullptr, m.find(1, 3));

	EXPECT_EQ(5, *m.find(0, 0));
	EXPECT_EQ(6, *m.find(0, 3));
	EXPECT_EQ(7, *m.find(1, 0));
	EXPECT_EQ(8, *m.find(1, 2));
	EXPECT_EQ(9, *m.find(1, 3));
}


void test_rank() {
	sparse::Matrix<int> m(2, 4);

	EXPECT_EQ(0, m.rank_row(0));
	EXPECT_EQ(0, m.rank_row(1));

	m.insert(0, 0, 5);
	m.insert(0, 3, 6);

	EXPECT_EQ(2, m.rank_row(0));
	EXPECT_EQ(1, m.rank_col(3));

	m.insert(1, 0, 7);
	m.insert(1, 2, 8);
	m.insert(1, 3, 9);

	// rank
	EXPECT_EQ(2, m.rank_row(0));
	EXPECT_EQ(3, m.rank_row(1));
	EXPECT_EQ(2, m.rank_col(0));
	EXPECT_EQ(0, m.rank_col(1));
	EXPECT_EQ(1, m.rank_col(2));
	EXPECT_EQ(2, m.rank_col(3));

}

void test_get() {
	sparse::Matrix<int> m(2, 4);

	m.insert(1, 0, 5);
	m.insert(0, 3, 6);

	// get_or
	EXPECT_EQ(3, m.get(0, 1, 3));
	EXPECT_EQ(6, m.get(0, 3));
}


void test_clear() {
	sparse::Matrix<int> m(2, 4);

	// insert
	m.insert(0, 0, 5);
	m.insert(0, 3, 6);
	m.insert(1, 0, 7);
	m.insert(1, 2, 8);
	m.insert(1, 3, 9);

	// clear_col
	EXPECT_EQ(2, m.clear_col(0));
	EXPECT_EQ((void*) nullptr, m.find(0, 0));
	EXPECT_EQ((void*) nullptr, m.find(1, 0));
	EXPECT_NE((void*) nullptr, m.find(0, 3));
	EXPECT_NE((void*) nullptr, m.find(1, 2));

	// clear_row
	EXPECT_EQ(2, m.clear_row(1));
	EXPECT_EQ((void*) nullptr, m.find(1, 2));
	EXPECT_EQ((void*) nullptr, m.find(1, 3));
	EXPECT_NE((void*) nullptr, m.find(0, 3));

	// rank
	EXPECT_EQ(1, m.rank_row(0));
	EXPECT_EQ(0, m.rank_row(1));
	EXPECT_EQ(0, m.rank_col(0));
	EXPECT_EQ(0, m.rank_col(1));
	EXPECT_EQ(0, m.rank_col(2));
	EXPECT_EQ(1, m.rank_col(3));

	m.clear();
	EXPECT_EQ(0, m.rank_row(0));
	EXPECT_EQ(0, m.rank_col(3));
}


void test_create_from() {
	std::vector<int> vec{1, 2, 3, 4, 5, 6};
	sparse::Matrix<int> m(3, 2);

	m.create_from([&vec](int row, int col) {
		auto& value = vec[row * 2 + col];
		return value % 2 == 1 ? &value : nullptr;
	});

	EXPECT_EQ(1, m.get(0, 0));
	EXPECT_EQ(0, m.get(0, 1));
	EXPECT_EQ(3, m.get(1, 0));
	EXPECT_EQ(0, m.get(1, 1));
	EXPECT_EQ(5, m.get(2, 0));
	EXPECT_EQ(0, m.get(2, 1));

}


int main() {
	test_resize();
	test_insert();
	test_resize();
	test_get();
	test_clear();
	test_create_from();
}

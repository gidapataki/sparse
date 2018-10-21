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

#define EXPECT_EQ(u, v) \
	expect_eq(__LINE__, u, v)

#define EXPECT_NE(u, v) \
	expect_ne(__LINE__, u, v)


int main() {
	sparse::Matrix<int> m;

	EXPECT_EQ(0, m.row_size());
	EXPECT_EQ(0, m.col_size());

	m.resize(2, 4);
	EXPECT_EQ(2, m.row_size());
	EXPECT_EQ(4, m.col_size());
	EXPECT_EQ((void*) nullptr, m.find(1, 2));

	m.insert(0, 0, 5);
	m.insert(0, 3, 6);
	m.insert(1, 0, 7);
	m.insert(1, 2, 8);
	m.insert(1, 3, 9);

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

#pragma once

#include <vector>
#include <type_traits>


namespace sparse {

namespace detail {

template<typename F, typename... Ts>
typename std::enable_if<std::is_same<decltype(std::declval<F>()(std::declval<Ts>()...)), void>::value, bool>::type
invoke(F func, Ts&&... ts) {
    func(std::forward<Ts>(ts)...);
    return true;
}

template<typename F, typename... Ts>
typename std::enable_if<!std::is_same<decltype(std::declval<F>()(std::declval<Ts>()...)), void>::value, bool>::type
invoke(F func, Ts&&... ts) {
    return func(std::forward<Ts>(ts)...);
}

} // namespace detail


using Index = std::size_t;
using Rank = std::size_t;

struct Node {
	Node(const Node& other) = delete;
	Node& operator=(const Node& other) = delete;

	Node() {
		link(this, this);
	}

	~Node() {
		unlink(this);
	}

	Node(Node&& other) {
		if (is_linked(&other)) {
			link(this, other.next);
			link(other.prev, this);
			link(&other, &other);
		} else {
			link(this, this);
		}
	}

	Node& operator=(Node&& other) {
		if (&other != this) {
			unlink(this);
			if (is_linked(&other)) {
				link(this, other.next);
				link(other.prev, this);
				link(&other, &other);
			}
		}

		return *this;
	}

	static bool is_linked(Node* u) {
		return u != u->next;
	}

	static void link(Node* u, Node* v) {
		u->next = v;
		v->prev = u;
	}

	static void unlink(Node* u) {
		if (is_linked(u)) {
			link(u->prev, u->next);
			link(u, u);
		}
	}

	void insert_before(Node* u) {
		if (u == this) {
			return;
		}

		link(prev, next);
		link(u->prev, this);
		link(this, u);
	}

	Node* prev;
	Node* next;
	Index index = -1;
};



template<typename Value>
class Matrix {
private:
	struct Head {
		Node node;
		Rank rank = 0;
	};

	struct Cell {
		Node in_row;
		Node in_col;
		Value value = {};
	};

public:
	using CellPtr = Cell*;

	Matrix() = default;
	Matrix(Index rows, Index cols) {
		resize(rows, cols);
	}

	Matrix(const Matrix&) = delete;
	Matrix& operator=(const Matrix&) = delete;

	Matrix(Matrix&& other) = default;
	Matrix& operator=(Matrix&&) = default;

	~Matrix() {
		clear();
	}

	Index row_size() const {
		return rows_.size();
	}

	Index col_size() const {
		return cols_.size();
	}

	void resize(Index rows, Index cols) {
		if (rows < rows_.size()) {
			for (auto row = rows; row < rows_.size(); ++row) {
				clear_row(row);
			}
		}

		if (cols < cols_.size()) {
			for (auto col = cols; col < cols_.size(); ++col) {
				clear_col(col);
			}
		}

		rows_.resize(rows);
		cols_.resize(cols);
	}

	Value get(Index row, Index col, Value value={}) {
		auto* cell = find_cell(row, col);
		if (cell) {
			return cell->value;
		}
		return value;
	}

	Value* find(Index row, Index col) {
		auto* cell = find_cell(row, col);
		if (cell) {
			return &cell->value;
		}
		return nullptr;
	}

	CellPtr find_cell(Index row, Index col) {
		Head& row_head = rows_[row];
		Head& col_head = cols_[col];

		if (row_head.rank > 0) {
			auto* cell = row_head.rank < col_head.rank
				? find_in_row(row, col)
				: find_in_col(row, col);

			return cell;
		}
		return nullptr;
	}

	CellPtr row_next(CellPtr cell) {
		if (cell == nullptr) {
			return nullptr;
		}

		auto& head = rows_[cell->in_col.index];
		auto* p = cell->in_row.next;
		if (p == &head.node) {
			return nullptr;
		}

		return from_row(p);
	}

	CellPtr row_prev(CellPtr cell) {
		if (cell == nullptr) {
			return nullptr;
		}

		auto& head = rows_[cell->in_col.index];
		auto* p = cell->in_row.prev;
		if (p == &head.node) {
			return nullptr;
		}

		return from_row(p);
	}

	CellPtr col_next(CellPtr cell) {
		if (cell == nullptr) {
			return nullptr;
		}

		auto& head = cols_[cell->in_row.index];
		auto* p = cell->in_col.next;
		if (p == &head.node) {
			return nullptr;
		}

		return from_col(p);
	}

	CellPtr col_prev(CellPtr cell) {
		if (cell == nullptr) {
			return nullptr;
		}

		auto& head = cols_[cell->in_row.index];
		auto* p = cell->in_col.prev;
		if (p == &head.node) {
			return nullptr;
		}

		return from_col(p);
	}

	Index get_row(CellPtr cell) {
		if (cell == nullptr) {
			return -1;
		}

		return cell->in_col.index;
	}

	Index get_col(CellPtr cell) {
		if (cell == nullptr) {
			return -1;
		}

		return cell->in_row.index;
	}

	Value& at(CellPtr cell) {
		return cell->value;
	}

	void insert(Index row, Index col, const Value& value) {
		access(row, col) = value;
	}

	Rank clear_row(Index index) {
		Rank removed = 0;
		auto& head = rows_[index];
		for (Node* p = head.node.next; p != &head.node; ) {
			auto* cell = from_row(p);
			p = p->next;
			remove_cell(cell);
			++removed;
		}
		return removed;
	}

	Rank clear_col(Index index) {
		Rank removed = 0;
		auto& head = cols_[index];
		for (Node* p = head.node.next; p != &head.node; ) {
			auto* cell = from_col(p);
			p = p->next;
			remove_cell(cell);
			++removed;
		}
		return removed;
	}

	Rank clear() {
		Rank removed = 0;
		for (Index i = 0; i < rows_.size(); ++i) {
			removed += clear_row(i);
		}
		return removed;
	}

	Rank rank_row(Index index) const {
		return rows_[index].rank;
	}

	Rank rank_col(Index index) const {
		return cols_[index].rank;
	}

	template<typename Func>
	void create_from(Func func) {
		clear();
		for (Index row = 0; row < rows_.size(); ++row) {
			auto& row_head = rows_[row];
			for (Index col = 0; col < cols_.size(); ++col) {
				auto* value = func(row, col);
				if (!value) {
					continue;
				}

				auto* cell = new Cell();
				auto& col_head = cols_[col];

				cell->value = *value;
				cell->in_row.index = col;
				cell->in_col.index = row;
				cell->in_row.insert_before(&row_head.node);
				cell->in_col.insert_before(&col_head.node);
				++row_head.rank;
				++col_head.rank;
			}
		}
	}

	template<typename Func>
	void for_each_in_row(Index row, Func func) {
		auto& head = rows_[row];
		for (Node* p = head.node.next; p != &head.node; p = p->next) {
			auto* cell = from_row(p);
			auto col = p->index;
			if (!detail::invoke(func, row, col, cell->value)) {
				break;
			}
		}
	}

	template<typename Func>
	void for_each_in_col(Index col, Func func) {
		auto& head = cols_[col];
		for (Node* p = head.node.next; p != &head.node; p = p->next) {
			auto* cell = from_col(p);
			auto row = p->index;
			if (!detail::invoke(func, row, col, cell->value)) {
				break;
			}
		}
	}

	template<typename Func>
	void for_each(Func func) {
		for (Index row = 0; row < rows_.size(); ++row) {
			auto& head = rows_[row];
			for (Node* p = head.node.next; p != &head.node; p = p->next) {
				auto* cell = from_row(p);
				auto col = p->index;
				if (!detail::invoke(func, row, col, cell->value)) {
					break;
				}
			}
		}
	}

	template<typename Func>
	void remove_in_row_if(Index row, Func func) {
		auto& head = rows_[row];
		for (Node* p = head.node.next; p != &head.node; ) {
			auto* cell = from_row(p);
			auto col = p->index;
			p = p->next;

			if (func(row, col, cell->value)) {
				remove_cell(cell);
			}
		}
	}

	template<typename Func>
	void remove_in_col_if(Index col, Func func) {
		auto& head = cols_[col];
		for (Node* p = head.node.next; p != &head.node; ) {
			auto* cell = from_col(p);
			auto row = p->index;
			p = p->next;

			if (func(row, col, cell->value)) {
				remove_cell(cell);
			}
		}
	}

	template<typename Func>
	void remove_if(Func func) {
		for (Index row = 0; row < rows_.size(); ++row) {
			auto& head = rows_[row];
			for (Node* p = head.node.next; p != &head.node; ) {
				auto* cell = from_row(p);
				auto col = p->index;
				p = p->next;

				if (func(row, col, cell->value)) {
					remove_cell(cell);
				}
			}
		}
	}


private:
	static Cell* from_row(Node* node) {
		return reinterpret_cast<Cell*>(node);
	}

	static Cell* from_col(Node* node) {
		return reinterpret_cast<Cell*>(node - 1);
	}

	void remove_cell(Cell* cell) {
		--rows_[cell->in_col.index].rank;
		--cols_[cell->in_row.index].rank;
		delete cell;
	}

	Cell* find_in_row(Index row, Index col) {
		Head& head = rows_[row];
		if (head.rank == 0) {
			return nullptr;
		}

		for (Node* p = head.node.next; p != &head.node; p = p->next) {
			if (p->index == col) {
				auto* cell = from_row(p);
				return cell;
			}
		}
		return nullptr;
	}

	Cell* find_in_col(Index row, Index col) {
		Head& head = cols_[col];
		if (head.rank == 0) {
			return nullptr;
		}

		for (Node* p = head.node.next; p != &head.node; p = p->next) {
			if (p->index == row) {
				auto* cell = from_col(p);
				return cell;
			}
		}
		return nullptr;
	}

	Value& access(Index row, Index col) {
		auto* old_cell = find_cell(row, col);
		if (old_cell) {
			return old_cell->value;
		}

		auto* new_cell = new Cell();
		Head& row_head = rows_[row];
		Head& col_head = cols_[col];

		Node* p = nullptr;
		for (p = row_head.node.next; p != &row_head.node; p = p->next) {
			auto* cell = from_row(p);
			if (cell->in_row.index > col) {
				break;
			}
		}
		new_cell->in_row.insert_before(p);
		new_cell->in_row.index = col;

		for (p = col_head.node.next; p != &col_head.node; p = p->next) {
			auto* cell = from_col(p);
			if (cell->in_col.index > row) {
				break;
			}
		}
		new_cell->in_col.insert_before(p);
		new_cell->in_col.index = row;

		++row_head.rank;
		++col_head.rank;

		return new_cell->value;
	}

	std::vector<Head> rows_;
	std::vector<Head> cols_;
};


} // namespace sparse

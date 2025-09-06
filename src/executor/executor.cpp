// /src/executor/executor.cpp

#include "executor/executor.h"
#include "executor/operators.h"

namespace venus {
namespace executor {

	ResultSet Executor::ExecutePlan(const planner::PlanNode* plan) {
		if (plan == nullptr) {
			return ResultSet::Failure("Executor error: Plan node is null");
		}

		std::unique_ptr<AbstractExecutor> root;

		try {
			root = BuildExecutorTree(plan);
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor build failed: ") + e.what());
		}

		if (root == nullptr) {
			return ResultSet::Failure("Executor error: Failed to build executor tree");
		}

		try {
			root->Open();
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor open failed: ") + e.what());
		}

		std::unique_ptr<TupleSet> tuple_set = nullptr;
		size_t num_rows = 0;
		OperatorOutput out;

		try {
			while (root->Next(&out)) {
				if (out.type_ == OperatorOutput::OutputType::TUPLE) {
					if (!tuple_set) {
						if (out.schema_ == nullptr) {
							static Schema empty_schema;
							tuple_set = std::make_unique<TupleSet>(empty_schema);
						} else {
							tuple_set = std::make_unique<TupleSet>(*out.schema_);
						}
					}
					tuple_set->AddTuple(out.tuple_);
					num_rows++;
				} else if (out.type_ == OperatorOutput::OutputType::MESSAGE) {
					root->Close();
					return ResultSet(out.ok_, out.message_ + " (" + std::to_string(num_rows) + " rows affected)");
				}
			}
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor next failed: ") + e.what());
		}

		try {
			root->Close();
		} catch (const std::exception& e) {
			return ResultSet::Failure(std::string("Executor Close() failed: ") + e.what());
		}

		if (num_rows > 0) {
			return ResultSet::Data(std::move(tuple_set));
		}

		return ResultSet::Success("Ok");
	}

	std::unique_ptr<AbstractExecutor> Executor::BuildExecutorTree(const planner::PlanNode* plan) {
		if (!plan)
			return nullptr;

		switch (plan->type_) {
		case PlanNodeType::SEQ_SCAN: {
			auto p = static_cast<const planner::SeqScanPlanNode*>(plan);
			return std::make_unique<SeqScanExecutor>(context_, p);
		}
		case PlanNodeType::PROJECTION: {
			auto p = static_cast<const planner::ProjectionPlanNode*>(plan);
			if (p->children_.size() != 1) {
				throw std::runtime_error("Projection expects exactly one child");
			}
			auto child = BuildExecutorTree(p->children_[0].get());
			return std::make_unique<ProjectionExecutor>(context_, p, std::move(child));
		}

		case PlanNodeType::INSERT: {
			auto p = static_cast<const planner::InsertPlanNode*>(plan);
			return std::make_unique<InsertExecutor>(context_, p);
		}

		case PlanNodeType::CREATE_TABLE: {
			auto p = static_cast<const planner::CreateTablePlanNode*>(plan);
			return std::make_unique<CreateTableExecutor>(context_, p);
		}

		case PlanNodeType::DROP_TABLE: {
			auto p = static_cast<const planner::DropTablePlanNode*>(plan);
			return std::make_unique<DropTableExecutor>(context_, p);
		}

		case PlanNodeType::CREATE_DATABASE:
		case PlanNodeType::DROP_DATABASE:
		case PlanNodeType::USE_DATABASE:
		case PlanNodeType::SHOW_DATABASES: {
			auto p = static_cast<const planner::DatabaseOpPlanNode*>(plan);
			return std::make_unique<DatabaseOpExecutor>(context_, p);
		}

		case PlanNodeType::SHOW_TABLES: {
			auto p = static_cast<const planner::ShowTablesPlanNode*>(plan);
			return std::make_unique<ShowTablesExecutor>(context_, p);
		}

		default:
			throw std::runtime_error("Executor: Unsupported plan node type");
		}
	}

	void Executor::PrintResultSet(const ResultSet& result_set) {
		if (result_set.success_) {
			if (result_set.data_) {
				const TupleSet& tuple_set = *result_set.data_;
				const Schema& schema = tuple_set.GetSchema();
				const auto& tuples = tuple_set.GetTuples();

				if (tuples.empty()) {
					std::cout << "No data found." << std::endl;
					return;
				}

				std::cout << std::endl;
				for (size_t i = 0; i < schema.GetColumnCount(); i++) {
					const Column& col = schema.GetColumn(i);
					std::cout << std::setw(18) << std::left << col.GetName();
					if (i < schema.GetColumnCount() - 1) {
						std::cout << " | ";
					}
				}
				std::cout << std::endl;

				for (size_t i = 0; i < schema.GetColumnCount(); i++) {
					std::cout << std::string(18, '-');
					if (i < schema.GetColumnCount() - 1) {
						std::cout << "-+-";
					}
				}
				std::cout << std::endl;

				for (const auto& tuple : tuples) {
					for (size_t i = 0; i < schema.GetColumnCount(); i++) {
						const Column& col = schema.GetColumn(i);

						const char* raw_value = tuple.GetValue(i, &schema);
						std::string value;

						if (raw_value != nullptr) {
							switch (col.GetType()) {
							case ColumnType::INT:
								value = std::to_string(*reinterpret_cast<const int*>(raw_value));
								break;
							case ColumnType::FLOAT:
								value = std::to_string(*reinterpret_cast<const float*>(raw_value));
								break;
							case ColumnType::CHAR:
								value = std::string(raw_value);
								break;
							default:
								value = "UNK";
								break;
							}
						} else {
							value = "NULL";
						}

						std::cout << std::setw(18) << std::left << value;
						if (i < schema.GetColumnCount() - 1) {
							std::cout << " | ";
						}
					}
					std::cout << std::endl;
				}
				std::cout << std::endl;
			}
			std::cout << result_set.message_ << std::endl;
		} else {
			std::cout << "Failed to execute query: " << result_set.message_ << std::endl;
		}
	}
}
}
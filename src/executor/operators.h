// /src/executor/operators.h

/**
 *  Volcano Model has 3 methods - Open, Next, Close
 * Open() - propagate downwards, each executor opens its child executor
 * Next() - Each executors pulls tuples from its child, and produces them as output
 * Close() - propagate upwards, each executor closes its child executor
 */

#pragma once

#include "executor/executor.h"

namespace venus {
namespace executor {
	class SeqScanExecutor : public AbstractExecutor {
	public:
		SeqScanExecutor(ExecutorContext* context, const planner::SeqScanPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override {
			table_heap_ = new table::TableHeap(
			    context_->bpm_,
			    plan_->table_ref_->GetSchema(),
			    plan_->table_ref_->GetFirstPageId());
		}

		bool Next(Tuple* tuple) override {
			if (table_heap_ == nullptr) {
				throw std::runtime_error("Operator error: TableHeap is not initialized - SeqScanExecutor");
			}

			// contd here
			
			return false;
		}

		void Close() override {
			if (table_heap_ != nullptr) {
				delete table_heap_;
				table_heap_ = nullptr;
			}
		}

		const Schema& GetOutputSchema() const override {
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::SeqScanPlanNode* plan_;
	};

	class ProjectionExecutor : public AbstractExecutor {
	public:
		ProjectionExecutor(ExecutorContext* context, const planner::ProjectionPlanNode* plan,
		    std::unique_ptr<AbstractExecutor> child)
		    : AbstractExecutor(context)
		    , plan_(plan)
		    , child_(std::move(child)) { }

		void Open() override {
			if (child_) {
				child_->Open();
			}
		}

		bool Next(Tuple* tuple) override {
			LOG("ProjectionExecutor::Next");

			if (child_) {
				return child_->Next(tuple);
			}
			return false;
		}

		void Close() override {
			if (child_) {
				child_->Close();
			}
		}

		const Schema& GetOutputSchema() const override {
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::ProjectionPlanNode* plan_;
		std::unique_ptr<AbstractExecutor> child_;
	};

	class InsertExecutor : public AbstractExecutor {
	public:
		InsertExecutor(ExecutorContext* context, const planner::InsertPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override {
		}

		bool Next(Tuple* tuple) override {
			LOG("InsertExecutor::Next");
			return false;
		}

		void Close() override {
		}

		const Schema& GetOutputSchema() const override {
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::InsertPlanNode* plan_;
	};

	class CreateTableExecutor : public AbstractExecutor {
	public:
		CreateTableExecutor(ExecutorContext* context, const planner::CreateTablePlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override {
		}

		bool Next(Tuple* tuple) override {
			LOG("CreateTableExecutor::Next");
			return false;
		}

		void Close() override {
		}

		const Schema& GetOutputSchema() const override {
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::CreateTablePlanNode* plan_;
	};

	class DropTableExecutor : public AbstractExecutor {
	public:
		DropTableExecutor(ExecutorContext* context, const planner::DropTablePlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override {
		}

		bool Next(Tuple* tuple) override {
			LOG("DropTableExecutor::Next");
			return false;
		}

		void Close() override {
		}

		const Schema& GetOutputSchema() const override {
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::DropTablePlanNode* plan_;
	};

	class ShowTablesExecutor : public AbstractExecutor {
	public:
		ShowTablesExecutor(ExecutorContext* context, const planner::ShowTablesPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override {
		}

		bool Next(Tuple* tuple) override {
			LOG("ShowTablesExecutor::Next");
			return false;
		}

		void Close() override {
		}

		const Schema& GetOutputSchema() const override {
			// TODO: Return proper schema
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::ShowTablesPlanNode* plan_;
	};

	class DatabaseOpExecutor : public AbstractExecutor {
	public:
		DatabaseOpExecutor(ExecutorContext* context, const planner::DatabaseOpPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override {
		}

		bool Next(Tuple* tuple) override {
			LOG("DatabaseOpExecutor::Next");

			return false;
		}

		void Close() override {
		}

		const Schema& GetOutputSchema() const override {
			static Schema empty_schema;
			return empty_schema;
		}

	private:
		const planner::DatabaseOpPlanNode* plan_;
	};

} // namespace executor
} // namespace venus
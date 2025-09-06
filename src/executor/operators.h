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
		    , plan_(plan)
		    , table_heap_(nullptr)
		    , is_open_(false) { }

		void Open() override {
			if (is_open_)
				return;

			table_heap_ = new table::TableHeap(
			    context_->bpm_,
			    plan_->table_ref_->GetSchema(),
			    plan_->table_ref_->GetTableId());

			curr_iterator_ = std::make_unique<table::TableHeap::Iterator>(table_heap_->begin());
			end_iterator_ = std::make_unique<table::TableHeap::Iterator>(table_heap_->end());
			is_open_ = true;
		}

		bool Next(OperatorOutput* out) override {
			if (!is_open_)
				return false;

			if (*curr_iterator_ != *end_iterator_) {
				out->ResetTuple();
				out->type_ = OperatorOutput::OutputType::TUPLE;
				out->tuple_ = **curr_iterator_;
				out->schema_ = plan_->table_ref_->GetSchema();
				out->ok_ = true;
				++(*curr_iterator_);
				return true;
			}

			return false;
		}

		void Close() override {
			if (!is_open_)
				return;

			curr_iterator_.reset();
			end_iterator_.reset();
			delete table_heap_;
			table_heap_ = nullptr;
			is_open_ = false;
		}

	private:
		const planner::SeqScanPlanNode* plan_;
		table::TableHeap* table_heap_;
		std::unique_ptr<table::TableHeap::Iterator> curr_iterator_;
		std::unique_ptr<table::TableHeap::Iterator> end_iterator_;
		bool is_open_;
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

		bool Next(OperatorOutput* out) override {
			if (!child_) {
				return false;
			}

			OperatorOutput child_out;
			if (!child_->Next(&child_out)) {
				return false;
			}

			if (child_out.type_ == OperatorOutput::OutputType::MESSAGE) {
				*out = std::move(child_out);
				return true;
			}

			if (child_out.type_ != OperatorOutput::OutputType::TUPLE || child_out.schema_ == nullptr) {
				throw std::runtime_error("ProjectionExecutor: child did not return a valid tuple");
			}

			const Schema* child_schema = child_out.schema_;
			std::vector<const char*> values;
			Schema output_schema;

			for (size_t i = 0; i < plan_->column_refs_.size(); i++) {
				const auto& col_ref = plan_->column_refs_[i];
				bool found = false;

				for (size_t j = 0; j < child_schema->GetColumnCount(); j++) {
					const Column& child_col = child_schema->GetColumn(j);
					if (child_col.GetName() == col_ref.GetName()) {
						const char* value = child_out.tuple_.GetValue(j, child_schema);
						values.push_back(value);

						output_schema.AddColumn(child_col.GetName(), child_col.GetType(), child_col.IsPrimary(), i);
						found = true;
						break;
					}
				}

				if (!found) {
					throw std::runtime_error("ProjectionExecutor: Column '" + col_ref.GetName() + "' not found in child schema");
				}
			}

			Tuple projected_tuple(values, &output_schema);

			out->ResetTuple();
			out->type_ = OperatorOutput::OutputType::TUPLE;
			out->tuple_ = projected_tuple;
			out->schema_ = new Schema(output_schema);
			out->ok_ = true;

			return true;
		}

		void Close() override {
			if (child_) {
				child_->Close();
			}
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

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			return false;
		}

		void Close() override { }

	private:
		const planner::InsertPlanNode* plan_;
	};

	class CreateTableExecutor : public AbstractExecutor {
	public:
		CreateTableExecutor(ExecutorContext* context, const planner::CreateTablePlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			return false;
		}

		void Close() override { }

	private:
		const planner::CreateTablePlanNode* plan_;
	};

	class DropTableExecutor : public AbstractExecutor {
	public:
		DropTableExecutor(ExecutorContext* context, const planner::DropTablePlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			return false;
		}

		void Close() override { }

	private:
		const planner::DropTablePlanNode* plan_;
	};

	class ShowTablesExecutor : public AbstractExecutor {
	public:
		ShowTablesExecutor(ExecutorContext* context, const planner::ShowTablesPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			LOG("ShowTablesExecutor::Next");
			return false;
		}

		void Close() override { }

	private:
		const planner::ShowTablesPlanNode* plan_;
	};

	class DatabaseOpExecutor : public AbstractExecutor {
	public:
		DatabaseOpExecutor(ExecutorContext* context, const planner::DatabaseOpPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			return false;
		}

		void Close() override { }

	private:
		const planner::DatabaseOpPlanNode* plan_;
	};

} // namespace executor
} // namespace venus
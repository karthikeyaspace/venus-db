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
			    plan_->table_ref_->GetFirstPageId());

			curr_iterator_ = std::make_unique<table::TableHeap::Iterator>(table_heap_->begin());
			end_iterator_ = std::make_unique<table::TableHeap::Iterator>(table_heap_->end());
			is_open_ = true;
		}

		bool Next(OperatorOutput* out) override {
			if (!is_open_)
				return false;

			if (*curr_iterator_ != *end_iterator_) {
				out->SetResponse("", OperatorOutput::OutputType::TUPLE, true, **curr_iterator_, plan_->table_ref_->GetSchema());
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

			Schema* output_schema = new Schema();

			for (size_t i = 0; i < plan_->column_refs_.size(); i++) {
				const auto& col_ref = plan_->column_refs_[i];
				bool found = false;

				for (size_t j = 0; j < child_schema->GetColumnCount(); j++) {
					const Column& child_col = child_schema->GetColumn(j);
					if (child_col.GetName() == col_ref.GetName()) {
						const char* value = child_out.tuple_.GetValue(j, child_schema);
						values.push_back(value);

						output_schema->AddColumn(child_col.GetName(), child_col.GetType(), child_col.IsPrimary(), i);
						found = true;
						break;
					}
				}

				if (!found) {
					delete output_schema;
					throw std::runtime_error("ProjectionExecutor: Column '" + col_ref.GetName() + "' not found in child schema");
				}
			}

			Tuple projected_tuple(values, output_schema);
			out->SetResponse("", OperatorOutput::OutputType::TUPLE, true, projected_tuple, output_schema);

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

		void Open() override {
			table_heap_ = new table::TableHeap(
			    context_->bpm_,
			    plan_->table_ref->GetSchema(),
			    plan_->table_ref->GetFirstPageId());
		}

		bool Next(OperatorOutput* out) override {
			if (plan_ == nullptr) {
				return false;
			}

			std::vector<std::string> insert_values;
			for (const ConstantType& v : plan_->values) {
				insert_values.push_back(v.value);
			}

			try {
				if (table_heap_->InsertTuple(insert_values)) {
					out->SetResponse("Inserted 1 row into " + plan_->table_ref->table_name, OperatorOutput::OutputType::MESSAGE, true);
					return true;
				} else {
					out->SetResponse("Failed to insert row into " + plan_->table_ref->table_name, OperatorOutput::OutputType::MESSAGE, false);
					return true;
				}
			} catch (const std::exception& e) {
				throw std::runtime_error("InsertExecutor::Next - Failed to insert tuple: " + std::string(e.what()));
			}
		}

		void Close() override { }

	private:
		const planner::InsertPlanNode* plan_;
		table::TableHeap* table_heap_;
	};

	class CreateTableExecutor : public AbstractExecutor {
	public:
		CreateTableExecutor(ExecutorContext* context, const planner::CreateTablePlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			try {
				context_->catalog_manager_->CreateTable(plan_->table_name_, &plan_->schema_);
				out->SetResponse("Table " + plan_->table_name_ + " created successfully.", OperatorOutput::OutputType::MESSAGE, true);
				return true;
			} catch (const std::exception& e) {
				throw std::runtime_error("CreateTableExecutor::Next - Failed to create table: " + std::string(e.what()));
			}
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
			throw std::runtime_error("DropTableExecutor::Next - Not implemented");
		}

		void Close() override { }

	private:
		const planner::DropTablePlanNode* plan_;
	};

	class ShowTablesExecutor : public AbstractExecutor {
	public:
		ShowTablesExecutor(ExecutorContext* context, const planner::ShowTablesPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan)
		    , seq_scan_executor_(nullptr)
		    , seq_scan_plan_(nullptr)
		    , output_schema_(nullptr) {
			output_schema_ = new Schema();
			output_schema_->AddColumn("table_name", ColumnType::CHAR, false, 0);
		}

		void Open() override {
			if (seq_scan_executor_) {
				return;
			}

			TableRef* master_tables_ref = context_->catalog_manager_->GetTableRef("master_tables");
			if (!master_tables_ref) {
				throw std::runtime_error("master_tables not found in catalog");
			}

			seq_scan_plan_ = std::make_unique<planner::SeqScanPlanNode>(master_tables_ref);
			seq_scan_executor_ = std::make_unique<SeqScanExecutor>(context_, seq_scan_plan_.get());
			seq_scan_executor_->Open();
		}

		bool Next(OperatorOutput* out) override {
			if (!seq_scan_executor_) {
				return false;
			}

			OperatorOutput child_out;
			if (!seq_scan_executor_->Next(&child_out)) {
				return false;
			}

			if (child_out.type_ != OperatorOutput::OutputType::TUPLE || child_out.schema_ == nullptr) {
				throw std::runtime_error("ShowTablesExecutor: SeqScan did not return a valid tuple");
			}

			const Schema* master_schema = child_out.schema_;
			const char* table_name_value = child_out.tuple_.GetValue(1, master_schema);

			if (table_name_value == nullptr) {
				throw std::runtime_error("ShowTablesExecutor: table_name value is null");
			}

			std::vector<const char*> values = { table_name_value };
			Tuple projected_tuple(values, output_schema_);
			out->SetResponse("", OperatorOutput::OutputType::TUPLE, true, projected_tuple, output_schema_);

			return true;
		}

		void Close() override {
			if (seq_scan_executor_) {
				seq_scan_executor_->Close();
				seq_scan_executor_.reset();
			}
			seq_scan_plan_.reset();
		}

		~ShowTablesExecutor() {
			if (output_schema_) {
				delete output_schema_;
			}
		}

	private:
		const planner::ShowTablesPlanNode* plan_;
		std::unique_ptr<SeqScanExecutor> seq_scan_executor_;
		std::unique_ptr<planner::SeqScanPlanNode> seq_scan_plan_;
		Schema* output_schema_;
	};

	class DatabaseOpExecutor : public AbstractExecutor {
	public:
		DatabaseOpExecutor(ExecutorContext* context, const planner::DatabaseOpPlanNode* plan)
		    : AbstractExecutor(context)
		    , plan_(plan) { }

		void Open() override { }

		bool Next(OperatorOutput* out) override {
			if (!plan_) {
				throw std::runtime_error("DatabaseOpExecutor::Next - Plan is null");
			}

			std::string db_name = plan_->database_name_;
			std::string db_path = std::string(venus::DATABASE_DIRECTORY) + "/" + db_name + ".db";

			switch (plan_->type_) {
			case PlanNodeType::CREATE_DATABASE: {
				std::filesystem::create_directories(venus::DATABASE_DIRECTORY);
				std::ofstream db_file(db_path);

				if (!db_file) {
					throw std::runtime_error("DatabaseOpExecutor::Next - Failed to create database file: " + db_path);
				}

				db_file.close();
				out->SetResponse("Ok", OperatorOutput::OutputType::MESSAGE, true);
				return true;
			}

			case PlanNodeType::DROP_DATABASE: {
				// delete that file
				if (std::filesystem::exists(db_path)) {
					std::filesystem::remove(db_path);
					out->SetResponse("Ok", OperatorOutput::OutputType::MESSAGE, true);
				} else {
					out->SetResponse("Database does not exist: " + db_name, OperatorOutput::OutputType::MESSAGE, false);
				}
				return true;
			}

			case PlanNodeType::USE_DATABASE: {
				out->SetResponse("Ok", OperatorOutput::OutputType::MESSAGE, true);
				return true;
			}

			case PlanNodeType::SHOW_DATABASES: {
				std::string message = "";
				for (const auto& entry : std::filesystem::directory_iterator(venus::DATABASE_DIRECTORY)) {
					if (entry.is_regular_file() && entry.path().extension() == ".db") {
						message += entry.path().stem().string() + "\n";
					}
				}
				out->SetResponse(message, OperatorOutput::OutputType::MESSAGE, true);
				return true;
			}

			default:
				out->SetResponse("Unsupported database operation", OperatorOutput::OutputType::MESSAGE, false);
				return true;
			}
		}

		void Close() override { }

	private:
		const planner::DatabaseOpPlanNode* plan_;
	};

} // namespace executor
} // namespace venus
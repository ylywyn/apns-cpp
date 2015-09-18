/*
* Copyright (C) ylywyn@gmail.com
* License: MIT
*/

#include <sstream>
#include <assert.h>
#include "RedisParser.h"
#include "Log.h"
RedisParser::RedisParser()
	: state(Start), bulk_size_(0), array_size_(0)
{
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::Parse(const char *ptr, size_t size, std::vector<RedisReply*>& rs, int& index)
{
	if (array_size_ == 0)
	{
		return parse_chunk(ptr, size, rs, index);
	}
	else
	{
		return parse_array(ptr, size, rs, index);
	}
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parse_array(const char *ptr, size_t size, std::vector<RedisReply*>& rs, int& index)
{
	size_t i = 0;
	size_t arr_idx = reply_->data_.size() - array_size_;

	for (; i < size; ++i)
	{
		char c = ptr[i];
		switch (state)
		{
		case Start:
			switch (c)
			{
			case bulkReply:
				state = BulkSize;
				bulk_size_ = 0;
				reply_->data_[arr_idx].clear();
				break;
			case integerReply:
				state = Integer;
				break;
			case stringReply:
				state = String;
				break;
			case errorReply:
				state = ErrorString;
				break;
			default:
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSize:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					bulk_size_ = atol(reply_->data_[arr_idx].c_str());
					state = BulkSizeLF;
					reply_->data_[arr_idx].clear();
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				bulk_size_ = atol(reply_->data_[arr_idx].c_str());
				state = BulkSizeLF;
				reply_->data_[arr_idx].clear();
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSizeLF:
			if (c == '\n')
			{
				if (bulk_size_ > 0)
				{
					reply_->data_[arr_idx].reserve(bulk_size_);
					long  available = size - i - 1;
					long  canRead = std::min(bulk_size_, available);

					if (canRead > 0)
					{
						reply_->data_[arr_idx].assign(ptr + i + 1, ptr + i + canRead + 1);
					}

					i += canRead;
					//是不是完整的 批量回复
					if (bulk_size_ > available)
					{
						bulk_size_ -= canRead;
						state = Bulk;
						return std::make_pair(i + 1, Incompleted);
					}
					else
					{   //完整
						state = BulkCR;
					}
				}
				else if (bulk_size_ == 0)
				{
					state = BulkCR;
				}
				else if (bulk_size_ < 0)
				{
					state = BulkCR;
					//reply_->type_ = RRT_NIL;  //只能内容为空 代表NIL了
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case Bulk: {
			assert(bulk_size_ > 0);

			long  available = size - i;
			long  canRead = std::min(available, bulk_size_);

			reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + i, ptr + canRead);
			bulk_size_ -= canRead;
			i += canRead - 1;

			if (bulk_size_ > 0)
			{
				return std::make_pair(i + 1, Incompleted);
			}
			else
			{
				state = BulkCR;
				if (size == i + 1)
				{
					return std::make_pair(i + 1, Incompleted);
				}
			}
			break;
		}
		case BulkCR:
			if (c == '\r')
			{
				state = BulkLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case BulkLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					rs[index] = reply_;
					if (++index == rs.size())
					{
						return std::make_pair(i + 1, CompletedAll);
					}
					else
					{
						//goto parse_chuank
						return std::make_pair(i + 1, Incompleted);
					}
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case Integer:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}

				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = IntegerLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = IntegerLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case IntegerLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					rs[index] = reply_;
					if (++index == rs.size())
					{
						return std::make_pair(i + 1, CompletedAll);
					}
					else
					{
						//goto parse_chuank
						return std::make_pair(i + 1, Incompleted);
					}
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case String:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = StringLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = StringLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case StringLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					rs[index] = reply_;
					if (++index == rs.size())
					{
						return std::make_pair(i + 1, CompletedAll);
					}
					else
					{
						//goto parse_chuank
						return std::make_pair(i + 1, Incompleted);
					}
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case ErrorString:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = ErrorLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = ErrorLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ErrorLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					rs[index] = reply_;
					if (++index == rs.size())
					{
						return std::make_pair(i + 1, CompletedAll);
					}
					else
					{
						//goto parse_chuank
						return std::make_pair(i + 1, Incompleted);
					}
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ArraySizeLF:
			state = Start;
			if (c != '\n')
			{
				return std::make_pair(i + 1, Error);
			}
			break;
		default:
			LOGFMTW("error! state: %d ", state);
			state = Start;
			return std::make_pair(i + 1, Error);
		}
	}

	return std::make_pair(i, Incompleted);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parse_chunk(const char *ptr, size_t size, std::vector<RedisReply*>& rs, int& index)
{
	size_t i = 0;
	for (; i < size; ++i)
	{
		char c = ptr[i];
		switch (state)
		{
		case Start:
			reply_ = RedisReply::NewReply();

			switch (c)
			{
			case bulkReply:
				state = BulkSize;
				reply_->type_ = RRT_BUILK;
				bulk_size_ = 0;
				reply_->data_[0].clear();
				break;
			case arrayReply:
				state = ArraySize;
				reply_->type_ = RRT_ARRAY;
				array_size_ = 0;
				reply_->data_[0].clear();
				break;
			case integerReply:
				state = Integer;
				reply_->type_ = RRT_INT;
				break;
			case stringReply:
				state = String;
				reply_->type_ = RRT_STRING;
				break;
			case errorReply:
				state = ErrorString;
				reply_->type_ = RRT_ERROR;
				break;
			default:
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSize:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					bulk_size_ = atol(reply_->data_[0].c_str());
					state = BulkSizeLF;
					reply_->data_[0].clear();
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				bulk_size_ = atol(reply_->data_[0].c_str());
				reply_->data_[0].clear();
				state = BulkSizeLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSizeLF:
			if (c == '\n')
			{
				if (bulk_size_ > 0)
				{
					reply_->data_[0].reserve(bulk_size_);
					long int available = size - i - 1;
					long int canRead = std::min(bulk_size_, available);

					if (canRead > 0)
					{
						reply_->data_[0].assign(ptr + i + 1, ptr + i + canRead + 1);
					}

					i += canRead;
					//是不是完整的 批量回复
					if (bulk_size_ > available)
					{
						bulk_size_ -= canRead;
						state = Bulk;
						return std::make_pair(i + 1, Incompleted);
					}
					else
					{   //完整
						state = BulkCR;
					}
				}
				else if (bulk_size_ == 0)
				{
					state = BulkCR;
				}
				else if (bulk_size_ < 0)
				{
					state = BulkCR;
					reply_->type_ = RRT_NIL;
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case Bulk: {
			assert(bulk_size_ > 0);

			long  available = size - i;
			long  canRead = std::min(available, bulk_size_);

			reply_->data_[0].insert(reply_->data_[0].end(), ptr + i, ptr + canRead);
			bulk_size_ -= canRead;
			i += canRead - 1;

			if (bulk_size_ > 0)
			{
				return std::make_pair(i + 1, Incompleted);
			}
			else
			{
				state = BulkCR;
				if (size == i + 1)
				{
					return std::make_pair(i + 1, Incompleted);
				}
			}
			break;
		}
		case BulkCR:
			if (c == '\r')
			{
				state = BulkLF;
			}
			else
			{
				LOGFMTW("error! bulk_size_: %d ", bulk_size_);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case BulkLF:
			if (c == '\n')
			{
				state = Start;
				rs[index] = reply_;
				if (++index == rs.size())
				{
					return std::make_pair(i + 1, CompletedAll);
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ArraySize:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					array_size_ = atol(reply_->data_[0].c_str());
					state = ArraySizeLF;
					reply_->data_[0].clear();
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				array_size_ = atol(reply_->data_[0].c_str());
				state = ArraySizeLF;
				reply_->data_[0].clear();
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ArraySizeLF:
			if (c == '\n')
			{
				state = Start;
				if (array_size_ > 0)
				{
					reply_->data_.resize(array_size_);
					if (i + 1 != size)
					{
						std::pair<size_t, ParseResult> parseResult = parse_array(ptr + i + 1, size - i - 1, rs, index);
						parseResult.first += i + 1;
						return parseResult;
					}
					else
					{
						return std::make_pair(i + 1, Incompleted);
					}
				}
				else if (array_size_ <= 0)
				{
					if (array_size_ < 0)
					{  // -1 is nil
						reply_->type_ = RRT_NIL;
					}

					rs[index] = reply_;
					if (++index == rs.size())
					{
						return std::make_pair(i + 1, CompletedAll);
					}
				}

			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case Integer:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}

				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					reply_->int_data_ = atol(reply_->data_[0].c_str());
					state = IntegerLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				reply_->int_data_ = atol(reply_->data_[0].c_str());
				state = IntegerLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case IntegerLF:
			if (c == '\n')
			{
				state = Start;
				rs[index] = reply_;
				if (++index == rs.size())
				{
					return std::make_pair(i + 1, CompletedAll);
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case String:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = StringLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = StringLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case StringLF:
			if (c == '\n')
			{
				state = Start;
				rs[index] = reply_;
				if (++index == rs.size())
				{
					return std::make_pair(i + 1, CompletedAll);
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case ErrorString:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = ErrorLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = ErrorLF;
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ErrorLF:
			if (c == '\n')
			{
				state = Start;
				rs[index] = reply_;
				if (++index == rs.size())
				{
					return std::make_pair(i + 1, CompletedAll);
				}
			}
			else
			{
				LOGFMTW("error! state: %d ", state);
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		default:
			LOGFMTW("error! state: %d ", state);
			state = Start;
			return std::make_pair(i + 1, Error);
		}
	}

	return std::make_pair(i, Incompleted);
}



//对应sub客户端的 parse, sub端,不能使用管线
std::pair<size_t, RedisParser::ParseResult> RedisParser::Parse(const char *ptr, size_t size, RedisReply* rs)
{
	if (array_size_ == 0)
	{
		return parse_chunk(ptr, size, rs);
	}
	else
	{
		return parse_array(ptr, size, rs);
	}
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parse_array(const char *ptr, size_t size, RedisReply* rs)
{
	size_t i = 0;
	size_t arr_idx = reply_->data_.size() - array_size_;

	for (; i < size; ++i)
	{
		char c = ptr[i];
		switch (state)
		{
		case Start:
			switch (c)
			{
			case bulkReply:
				state = BulkSize;
				bulk_size_ = 0;
				reply_->data_[arr_idx].clear();
				break;
			case integerReply:
				state = Integer;
				break;
			case stringReply:
				state = String;
				break;
			case errorReply:
				state = ErrorString;
				break;
			default:
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSize:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					bulk_size_ = atol(reply_->data_[arr_idx].c_str());
					state = BulkSizeLF;
					reply_->data_[arr_idx].clear();
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				bulk_size_ = atol(reply_->data_[arr_idx].c_str());
				state = BulkSizeLF;
				reply_->data_[arr_idx].clear();
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSizeLF:
			if (c == '\n')
			{
				if (bulk_size_ > 0)
				{
					reply_->data_[arr_idx].reserve(bulk_size_);
					long  available = size - i - 1;
					long  canRead = std::min(bulk_size_, available);

					if (canRead > 0)
					{
						reply_->data_[arr_idx].assign(ptr + i + 1, ptr + i + canRead + 1);
					}

					i += canRead;
					//是不是完整的 批量回复
					if (bulk_size_ > available)
					{
						bulk_size_ -= canRead;
						state = Bulk;
						return std::make_pair(i + 1, Incompleted);
					}
					else
					{   //完整
						state = BulkCR;
					}
				}
				else if (bulk_size_ == 0)
				{
					state = BulkCR;
				}
				else if (bulk_size_ < 0)
				{
					state = BulkCR;
					//reply_->type_ = RRT_NIL;  //只能内容为空 代表NIL了
				}
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case Bulk: {
			assert(bulk_size_ > 0);

			long  available = size - i;
			long  canRead = std::min(available, bulk_size_);

			reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + i, ptr + canRead);
			bulk_size_ -= canRead;
			i += canRead - 1;

			if (bulk_size_ > 0)
			{
				return std::make_pair(i + 1, Incompleted);
			}
			else
			{
				state = BulkCR;
				if (size == i + 1)
				{
					return std::make_pair(i + 1, Incompleted);
				}
			}
			break;
		}
		case BulkCR:
			if (c == '\r')
			{
				state = BulkLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case BulkLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					return std::make_pair(i + 1, CompletedOne);
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case Integer:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}

				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = IntegerLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = IntegerLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case IntegerLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					return std::make_pair(i + 1, CompletedOne);
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case String:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = StringLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = StringLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case StringLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					return std::make_pair(i + 1, CompletedOne);
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case ErrorString:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[arr_idx].insert(reply_->data_[arr_idx].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = ErrorLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = ErrorLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ErrorLF:
			if (c == '\n')
			{
				state = Start;
				if (--array_size_ == 0)
				{
					return std::make_pair(i + 1, CompletedOne);
				}
				else
				{
					++arr_idx;
				}
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ArraySizeLF:
			state = Start;
			if (c != '\n')
			{
				return std::make_pair(i + 1, Error);
			}
			break;
		default:
			state = Start;
			return std::make_pair(i + 1, Error);
		}
	}

	return std::make_pair(i, Incompleted);
}

std::pair<size_t, RedisParser::ParseResult> RedisParser::parse_chunk(const char *ptr, size_t size, RedisReply* rs)
{
	size_t i = 0;
	for (; i < size; ++i)
	{
		char c = ptr[i];
		switch (state)
		{
		case Start:
			reply_ = rs;
			switch (c)
			{
			case bulkReply:
				state = BulkSize;
				reply_->type_ = RRT_BUILK;
				bulk_size_ = 0;
				reply_->data_[0].clear();
				break;
			case arrayReply:
				state = ArraySize;
				reply_->type_ = RRT_ARRAY;
				array_size_ = 0;
				reply_->data_[0].clear();
				break;
			case integerReply:
				state = Integer;
				reply_->type_ = RRT_INT;
				break;
			case stringReply:
				state = String;
				reply_->type_ = RRT_STRING;
				break;
			case errorReply:
				state = ErrorString;
				reply_->type_ = RRT_ERROR;
				break;
			default:
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSize:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					bulk_size_ = atol(reply_->data_[0].c_str());
					state = BulkSizeLF;
					reply_->data_[0].clear();
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				bulk_size_ = atol(reply_->data_[0].c_str());
				state = BulkSizeLF;
				reply_->data_[0].clear();
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case BulkSizeLF:
			if (c == '\n')
			{
				if (bulk_size_ > 0)
				{
					reply_->data_[0].reserve(bulk_size_);
					long int available = size - i - 1;
					long int canRead = std::min(bulk_size_, available);

					if (canRead > 0)
					{
						reply_->data_[0].assign(ptr + i + 1, ptr + i + canRead + 1);
					}

					i += canRead;
					//是不是完整的 批量回复
					if (bulk_size_ > available)
					{
						bulk_size_ -= canRead;
						state = Bulk;
						return std::make_pair(i + 1, Incompleted);
					}
					else
					{   //完整
						state = BulkCR;
					}
				}
				else if (bulk_size_ == 0)
				{
					state = BulkCR;
				}
				else if (bulk_size_ < 0)
				{
					state = BulkCR;
					reply_->type_ = RRT_NIL;
				}
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case Bulk: {
			assert(bulk_size_ > 0);

			long  available = size - i;
			long  canRead = std::min(available, bulk_size_);

			reply_->data_[0].insert(reply_->data_[0].end(), ptr + i, ptr + canRead);
			bulk_size_ -= canRead;
			i += canRead - 1;

			if (bulk_size_ > 0)
			{
				return std::make_pair(i + 1, Incompleted);
			}
			else
			{
				state = BulkCR;
				if (size == i + 1)
				{
					return std::make_pair(i + 1, Incompleted);
				}
			}
			break;
		}
		case BulkCR:
			if (c == '\r')
			{
				state = BulkLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case BulkLF:
			if (c == '\n')
			{
				state = Start;
				return std::make_pair(i + 1, CompletedOne);

			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ArraySize:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					array_size_ = atol(reply_->data_[0].c_str());
					state = ArraySizeLF;
					reply_->data_[0].clear();
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				array_size_ = atol(reply_->data_[0].c_str());
				state = ArraySizeLF;
				reply_->data_[0].clear();
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ArraySizeLF:
			if (c == '\n')
			{
				state = Start;
				if (array_size_ > 0)
				{
					reply_->data_.resize(array_size_);
					if (i + 1 != size)
					{
						std::pair<size_t, ParseResult> parseResult = parse_array(ptr + i + 1, size - i - 1, rs);
						parseResult.first += i + 1;
						return parseResult;
					}
					else
					{
						return std::make_pair(i + 1, Incompleted);
					}
				}
				else if (array_size_ <= 0)
				{
					if (array_size_ < 0)
					{  // -1 is nil
						reply_->type_ = RRT_NIL;
					}
					state = Start;
					return std::make_pair(i + 1, CompletedOne);
				}

			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case Integer:
			if (isdigit(c) || c == '-')
			{
				int b = i;
				while (i < size  && *(ptr + i) != '\r')
				{
					++i;
				}

				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					reply_->int_data_ = atol(reply_->data_[0].c_str());
					state = IntegerLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				reply_->int_data_ = atol(reply_->data_[0].c_str());
				state = IntegerLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case IntegerLF:
			if (c == '\n')
			{
				state = Start;
				return std::make_pair(i + 1, CompletedOne);
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case String:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = StringLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = StringLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case StringLF:
			if (c == '\n')
			{
				state = Start;
				return std::make_pair(i + 1, CompletedOne);
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		case ErrorString:
			if (isCharNotControl(c))
			{
				int b = i;
				while (i < size && *(ptr + i) != '\r')
				{
					++i;
				}
				reply_->data_[0].insert(reply_->data_[0].end(), ptr + b, ptr + i);
				if (i < size)
				{
					state = ErrorLF;
				}
				else
				{
					return std::make_pair(i, Incompleted);
				}
			}
			else if (c == '\r')
			{
				state = ErrorLF;
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;
		case ErrorLF:
			if (c == '\n')
			{
				state = Start;
				return std::make_pair(i + 1, CompletedOne);
			}
			else
			{
				state = Start;
				return std::make_pair(i + 1, Error);
			}
			break;

		default:
			state = Start;
			return std::make_pair(i + 1, Error);
		}
	}

	return std::make_pair(i, Incompleted);
}



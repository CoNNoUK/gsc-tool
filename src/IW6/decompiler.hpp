// Copyright 2020 xensik. All rights reserved.
//
// Use of this source code is governed by a GNU GPLv3 license
// that can be found in the LICENSE file.

#ifndef _GSC_IW6_DECOMPILER_H_
#define _GSC_IW6_DECOMPILER_H_

namespace IW6
{

class decompiler : public gsc::decompiler
{
	std::unique_ptr<utils::byte_buffer> output_;
	std::vector<std::shared_ptr<decompiler_function>> functions_;
	bool in_waittill_;

public:
	decompiler();
	void decompile(std::vector<std::shared_ptr<function>>& functions);
	auto output() -> std::vector<std::uint8_t>;

private:
	void decompile_function(std::shared_ptr<decompiler_function> func);
	void decompile_statements(std::shared_ptr<decompiler_function> func);
	void decompile_blocks(std::shared_ptr<decompiler_function> func);

	void print_function(std::shared_ptr<decompiler_function> func);
	void print_statement(std::shared_ptr<statement> stmt);
	void print_label(const std::string& label);
};

} // namespace IW6

#endif // _GSC_IW6_DECOMPILER_H_

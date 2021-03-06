// Copyright 2020 xensik. All rights reserved.
//
// Use of this source code is governed by a GNU GPLv3 license
// that can be found in the LICENSE file.

#include "IW6.hpp"

namespace IW6
{

disassembler::disassembler(bool ida_output)
{
	ida_output_ = ida_output;
	output_ = std::make_unique<utils::byte_buffer>(0x100000);
}

void disassembler::disassemble(std::shared_ptr<utils::byte_buffer> script, std::shared_ptr<utils::byte_buffer> stack)
{
	script_ = script;
	stack_ = stack;
	output_->clear();
	functions_.clear();

	script_->seek(1); // skip magic opcode 0x34

	while (stack_->is_avail() && script_->is_avail())
	{
		auto func = std::make_shared<function>();
		func->index = script_->get_pos();
		func->size = stack_->read<std::uint32_t>();
		func->id = stack_->read<std::uint16_t>();
		func->name = "sub_"s + (func->id == 0 ? stack_->read_string() : resolver::token_name(func->id));
		functions_.push_back(func);

		this->dissasemble_function(func);
	}

	// fix local function calls here once we have all function addresses
	this->resolve_local_functions();
}

auto disassembler::output() -> std::vector<std::shared_ptr<function>>
{
	return functions_;
}

auto disassembler::output_buffer() -> std::vector<std::uint8_t>
{
	this->print_script_name(""); // TODO: add file name conversor

	for (auto& func : functions_)
	{
		this->print_function(func);

		for (auto& inst : func->instructions)
		{
			if (func->labels.find(inst->index) != func->labels.end())
			{
				this->print_label(func->labels[inst->index]);
			}

			this->print_instruction(inst);
		}
	}

	std::vector<std::uint8_t> output;
	output.resize(output_->get_pos());
	memcpy(output.data(), output_->get_buffer().data(), output.size());

	return output;
}

void disassembler::dissasemble_function(std::shared_ptr<function> func)
{
#ifdef DEV_DEBUG
	this->print_function(func);
#endif
	auto size = func->size;

	while (size > 0)
	{
		auto inst = std::make_shared<instruction>();
		inst->index = script_->get_pos();
		inst->opcode = script_->read<std::uint8_t>();
		inst->parent = func;
		func->instructions.push_back(inst);

		this->dissasemble_instruction(inst);

#ifdef DEV_DEBUG
		this->print_instruction(inst);
#endif

		size -= inst->size;
	}
}

void disassembler::dissasemble_instruction(std::shared_ptr<instruction> inst)
{
	switch (opcode(inst->opcode))
	{
	case opcode::OP_End:
	case opcode::OP_Return:
	case opcode::OP_GetUndefined:
	case opcode::OP_GetZero:
	case opcode::OP_waittillFrameEnd:
	case opcode::OP_EvalArray:
	case opcode::OP_EvalArrayRef:
	case opcode::OP_ClearArray:
	case opcode::OP_EmptyArray:
	case opcode::OP_AddArray:
	case opcode::OP_PreScriptCall:
	case opcode::OP_GetLevelObject:
	case opcode::OP_GetAnimObject:
	case opcode::OP_GetSelf:
	case opcode::OP_GetThisthread:
	case opcode::OP_GetLevel:
	case opcode::OP_GetGame:
	case opcode::OP_GetAnim:
	case opcode::OP_GetGameRef:
	case opcode::OP_inc:
	case opcode::OP_dec:
	case opcode::OP_bit_or:
	case opcode::OP_bit_ex_or:
	case opcode::OP_bit_and:
	case opcode::OP_equality:
	case opcode::OP_inequality:
	case opcode::OP_less:
	case opcode::OP_greater:
	case opcode::OP_waittill:
	case opcode::OP_notify:
	case opcode::OP_endon:
	case opcode::OP_voidCodepos:
	case opcode::OP_vector:
	case opcode::OP_less_equal:
	case opcode::OP_greater_equal:
	case opcode::OP_shift_left:
	case opcode::OP_shift_right:
	case opcode::OP_plus:
	case opcode::OP_minus:
	case opcode::OP_multiply:
	case opcode::OP_divide:
	case opcode::OP_mod:
	case opcode::OP_size:
	case opcode::OP_GetSelfObject:
	case opcode::OP_clearparams:
	case opcode::OP_checkclearparams:
	case opcode::OP_EvalLocalVariableRefCached0:
	case opcode::OP_EvalNewLocalVariableRefCached0:
	case opcode::OP_CastBool:
	case opcode::OP_BoolNot:
	case opcode::OP_BoolComplement:
	case opcode::OP_wait:
	case opcode::OP_DecTop:
	case opcode::OP_CastFieldObject:
	case opcode::OP_ClearLocalVariableFieldCached0:
	case opcode::OP_SetVariableField:
	case opcode::OP_SetLocalVariableFieldCached0:
		inst->size = 1;
		break;
	case opcode::OP_GetByte:
		inst->size = 2;
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint8_t>()));
		break;
	case opcode::OP_GetNegByte:
		inst->size = 2;
		inst->data.push_back(utils::string::va("%i", script_->read<std::int8_t>()));
		break;
	case opcode::OP_GetUnsignedShort:
		inst->size = 3;
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint16_t>()));
		break;
	case opcode::OP_GetNegUnsignedShort:
		inst->size = 3;
		inst->data.push_back(utils::string::va("%i", script_->read<std::int16_t>()));
		break;
	case opcode::OP_GetInteger:
		inst->size = 5;
		inst->data.push_back(utils::string::va("%i", script_->read<std::int32_t>()));
		break;
	case opcode::OP_GetFloat:
		inst->size = 5;
		inst->data.push_back(utils::string::va("%g", script_->read<float>()));
		break;
	case opcode::OP_GetVector:
		inst->size = 13;
		inst->data.push_back(utils::string::va("%g", script_->read<float>()));
		inst->data.push_back(utils::string::va("%g", script_->read<float>()));
		inst->data.push_back(utils::string::va("%g", script_->read<float>()));
		break;
	case opcode::OP_GetString:
	case opcode::OP_GetIString:
		inst->size = 5;
		script_->seek(4);
		inst->data.push_back(utils::string::va("\"%s\"", stack_->read_string().data()));
		break;
	case opcode::OP_GetAnimation:
		inst->size = 9;
		script_->seek(8);
		inst->data.push_back(utils::string::va("\"%s\"", stack_->read_string().data()));
		inst->data.push_back(utils::string::va("\"%s\"", stack_->read_string().data()));
		break;
	case opcode::OP_GetAnimTree:
		inst->size = 2;
		script_->seek(1);
		inst->data.push_back(utils::string::va("\"%s\"", stack_->read_string().data()));
		break;
	case opcode::OP_waittillmatch: // IW6 placeholder is 2 bytes???
		inst->size = 3;
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint16_t>()));
		break;
	case opcode::OP_waittillmatch2:
		// NOP
		break;
// VARIABLE
	case opcode::OP_EvalLocalVariableCached0:
	case opcode::OP_EvalLocalVariableCached1:
	case opcode::OP_EvalLocalVariableCached2:
	case opcode::OP_EvalLocalVariableCached3:
	case opcode::OP_EvalLocalVariableCached4:
	case opcode::OP_EvalLocalVariableCached5:
		inst->size = 1;
		break;
	case opcode::OP_SafeSetVariableFieldCached0:
		inst->size = 1;
		break;
	case opcode::OP_EvalNewLocalArrayRefCached0:
	case opcode::OP_CreateLocalVariable:
	case opcode::OP_RemoveLocalVariables:
	case opcode::OP_EvalLocalVariableCached:
	case opcode::OP_EvalLocalArrayCached:
	case opcode::OP_EvalLocalArrayRefCached0:
	case opcode::OP_EvalLocalArrayRefCached:
	case opcode::OP_SafeCreateVariableFieldCached:
	case opcode::OP_SafeSetVariableFieldCached:
	case opcode::OP_SafeSetWaittillVariableFieldCached:
	case opcode::OP_EvalLocalVariableRefCached:
	case opcode::OP_SetNewLocalVariableFieldCached0:
	case opcode::OP_SetLocalVariableFieldCached:
	case opcode::OP_ClearLocalVariableFieldCached:
	case opcode::OP_EvalLocalVariableObjectCached:
		inst->size = 2;
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint8_t>())); // var index
		break;
	case opcode::OP_EvalLevelFieldVariable:
	case opcode::OP_EvalAnimFieldVariable:
	case opcode::OP_EvalSelfFieldVariable:
	case opcode::OP_EvalFieldVariable:
	case opcode::OP_EvalLevelFieldVariableRef:
	case opcode::OP_EvalAnimFieldVariableRef:
	case opcode::OP_EvalSelfFieldVariableRef:
	case opcode::OP_EvalFieldVariableRef:
	case opcode::OP_ClearFieldVariable:
	case opcode::OP_SetLevelFieldVariableField:
	case opcode::OP_SetAnimFieldVariableField:
	case opcode::OP_SetSelfFieldVariableField:
		this->disassemble_field_variable(inst);
		break;
// POINTER
	case opcode::OP_ScriptFunctionCallPointer:
	case opcode::OP_ScriptMethodCallPointer:
		inst->size = 1;
		break;
	case opcode::OP_CallBuiltinPointer:
	case opcode::OP_CallBuiltinMethodPointer:
	case opcode::OP_ScriptThreadCallPointer:
	case opcode::OP_ScriptMethodThreadCallPointer:
	case opcode::OP_ScriptMethodChildThreadCallPointer:
	case opcode::OP_ScriptChildThreadCallPointer:
		inst->size = 2;
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint8_t>())); // arg num
		break;
// FAR CALL
	case opcode::OP_GetFarFunction:
	case opcode::OP_ScriptFarFunctionCall2:
	case opcode::OP_ScriptFarFunctionCall:
	case opcode::OP_ScriptFarMethodCall:
		this->disassemble_far_call(inst, false);
		break;
	case opcode::OP_ScriptFarThreadCall:
	case opcode::OP_ScriptFarChildThreadCall:
	case opcode::OP_ScriptFarMethodThreadCall:
	case opcode::OP_ScriptFarMethodChildThreadCall:
		this->disassemble_far_call(inst, true);
		break;
// LOCAL CALL
	case opcode::OP_GetLocalFunction:
	case opcode::OP_ScriptLocalFunctionCall2:
	case opcode::OP_ScriptLocalFunctionCall:
	case opcode::OP_ScriptLocalMethodCall:
		this->disassemble_local_call(inst, false);
		break;
	case opcode::OP_ScriptLocalThreadCall:
	case opcode::OP_ScriptLocalChildThreadCall:
	case opcode::OP_ScriptLocalMethodThreadCall:
	case opcode::OP_ScriptLocalMethodChildThreadCall:
		this->disassemble_local_call(inst, true);
		break;
// BUILTIN
	case opcode::OP_GetBuiltinFunction:
		this->disassemble_builtin_call(inst, false, false);
		break;
	case opcode::OP_GetBuiltinMethod:
		this->disassemble_builtin_call(inst, true, false);
		break;
	case opcode::OP_CallBuiltin:
		this->disassemble_builtin_call(inst, false, true);
		break;
	case opcode::OP_CallBuiltinMethod:
		this->disassemble_builtin_call(inst, true, true);
		break;
	case opcode::OP_CallBuiltin0:
	case opcode::OP_CallBuiltin1:
	case opcode::OP_CallBuiltin2:
	case opcode::OP_CallBuiltin3:
	case opcode::OP_CallBuiltin4:
	case opcode::OP_CallBuiltin5:
		this->disassemble_builtin_call(inst, false, false);
		break;
	case opcode::OP_CallBuiltinMethod0:
	case opcode::OP_CallBuiltinMethod1:
	case opcode::OP_CallBuiltinMethod2:
	case opcode::OP_CallBuiltinMethod3:
	case opcode::OP_CallBuiltinMethod4:
	case opcode::OP_CallBuiltinMethod5:
		this->disassemble_builtin_call(inst, true, false);
		break;
// JUMP
	case opcode::OP_jump:
		this->disassemble_jump(inst, false, false);
		break;
	case opcode::OP_jumpback:
		this->disassemble_jump(inst, false, true);
		break;
	case opcode::OP_JumpOnFalse:
		this->disassemble_jump(inst, true, false);
		break;
	case opcode::OP_JumpOnTrue:
		this->disassemble_jump(inst, true, false);
		break;
	case opcode::OP_JumpOnFalseExpr:
		this->disassemble_jump(inst, true, false);
		break;
	case opcode::OP_JumpOnTrueExpr:
		this->disassemble_jump(inst, true, false);
		break;
// SWITCH
	case opcode::OP_switch:
		this->disassemble_switch(inst);
		break;
	case opcode::OP_endswitch:
		this->disassemble_end_switch(inst);
		break;

	default:
		LOG_ERROR("Unhandled opcode (0x%X) at index '%04X'!", inst->opcode, inst->index);
		break;
	}
}

void disassembler::disassemble_builtin_call(std::shared_ptr<instruction> inst, bool method, bool arg_num)
{
	inst->size = arg_num ? 4 : 3;

	if (arg_num)
	{
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint8_t>()));
	}

	if (method)
	{
		inst->data.push_back(resolver::builtin_method_name(script_->read<std::uint16_t>()));
	}
	else
	{
		inst->data.push_back(resolver::builtin_func_name(script_->read<std::uint16_t>()));
	}
}

void disassembler::disassemble_local_call(std::shared_ptr<instruction> inst, bool thread)
{
	inst->size = thread ? 5 : 4;

	std::int32_t offset = this->disassemble_offset();

	inst->data.push_back(utils::string::va("%X", offset + inst->index + 1));

	if (thread)
	{
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint8_t>()));
	}
}

void disassembler::disassemble_far_call(std::shared_ptr<instruction> inst, bool thread)
{
	inst->size = thread ? 5 : 4;

	script_->seek(3); // pointer placeholder 24 bit offset

	if (thread)
	{
		inst->data.push_back(utils::string::va("%i", script_->read<std::uint8_t>()));
	}

	auto file_id = stack_->read<std::uint16_t>();
	auto file_name = file_id == 0 ? stack_->read_string() : resolver::file_name(file_id);
	auto func_id = stack_->read<std::uint16_t>();
	auto func_name = func_id == 0 ? stack_->read_string() : resolver::token_name(func_id);

	inst->data.push_back(file_name != "" ? file_name : utils::string::va("id#%i", file_id));
	inst->data.push_back(func_name != "" ? func_name : utils::string::va("id#%i", func_id));
}

void disassembler::disassemble_jump(std::shared_ptr<instruction> inst, bool expr, bool back)
{
	inst->size = (expr || back) ? 3 : 5;

	std::int32_t addr;
	std::string label;

	if (expr)
	{
		addr = inst->index + 3 + script_->read<std::int16_t>();
		label = utils::string::va("loc_%X", addr);
		inst->data.push_back(label);
	}
	else if (back)
	{
		addr = inst->index + 3 - script_->read<std::uint16_t>();
		label = utils::string::va("loc_%X", addr);
		inst->data.push_back(label);
	}
	else
	{
		addr = inst->index + 5 + script_->read<std::int32_t>();
		label = utils::string::va("loc_%X", addr);
		inst->data.push_back(label);
	}

	inst->parent->labels[addr] = label;
}

void disassembler::disassemble_field_variable(std::shared_ptr<instruction> inst)
{
	inst->size = 3;

	std::uint16_t field_id = script_->read<std::uint16_t>();
	std::string field_name = field_id > 38305 ? stack_->read_opaque_string() : resolver::token_name(field_id);

	inst->data.push_back(field_name != "" ? field_name : utils::string::va("id#%i", field_id));
}

void disassembler::disassemble_switch(std::shared_ptr<instruction> inst)
{
	inst->size = 5;

	std::int32_t addr = inst->index + 4 + script_->read<std::int32_t>();
	std::string label = utils::string::va("loc_%X", addr);

	inst->data.push_back(label);
	inst->parent->labels[addr] = label;
}

void disassembler::disassemble_end_switch(std::shared_ptr<instruction> inst)
{
	inst->size = 3;

	std::uint16_t case_num = script_->read<std::uint16_t>();
	inst->data.push_back(utils::string::va("%i", case_num));

	std::uint32_t internal_index = inst->index + 3;

	if (case_num)
	{
		for (auto i = case_num; i > 0; i--)
		{
			std::uint32_t case_label = script_->read<std::uint32_t>();

			if (case_label < 0x10000 && case_label > 0)
			{
				inst->data.push_back("case");
				inst->data.push_back(utils::string::va("\"%s\"", stack_->read_string().data()));
			}
			else if (case_label < 0x40000)
			{
				inst->data.push_back("default");
				stack_->read<std::uint16_t>(); // should be 01 00 (opaque string id)
			}
			else
			{
				inst->data.push_back("case");
				inst->data.push_back(utils::string::va("%i", case_label & 0xFFFF));
			}

			inst->size += 4;
			internal_index += 4;

			auto addr = this->disassemble_offset() + internal_index;
			std::string label = utils::string::va("loc_%X", addr);
			inst->data.push_back(label);

			inst->parent->labels[addr] = label;

			inst->size += 3;
			internal_index += 3;
		}
	}
}

auto disassembler::disassemble_offset() -> std::int32_t
{
	std::vector<std::uint8_t> bytes;

	bytes.resize(4);
	std::fill(bytes.begin(), bytes.end(), 0);

	for (int i = 0; i < 3; i++)
	{
		bytes[i] = script_->read<std::uint8_t>();
	}

	std::int32_t offset = *reinterpret_cast<std::int32_t*>(bytes.data());

	offset = (offset << 8) >> 10;

	return offset;
}

void disassembler::resolve_local_functions()
{
	for (auto& func : functions_)
	{
		for (auto& inst : func->instructions)
		{
			switch (opcode(inst->opcode))
			{
			case opcode::OP_GetLocalFunction:
			case opcode::OP_ScriptLocalFunctionCall:
			case opcode::OP_ScriptLocalFunctionCall2:
			case opcode::OP_ScriptLocalMethodCall:
			case opcode::OP_ScriptLocalThreadCall:
			case opcode::OP_ScriptLocalChildThreadCall:
			case opcode::OP_ScriptLocalMethodThreadCall:
			case opcode::OP_ScriptLocalMethodChildThreadCall:
				inst->data.at(0) = this->resolve_function(inst->data[0]);
				break;
			default:
				break;
			}
		}
	}
}

auto disassembler::resolve_function(const std::string& index) -> std::string
{
	if (utils::string::is_hex_number(index))
	{
		std::uint32_t idx = std::stoul(index, nullptr, 16);

		for (auto& func : functions_)
		{
			if (func->index == idx)
			{
				return func->name;
			}
		}

		LOG_ERROR("Couldn't resolve function name at index '0x%04X'!", idx);
		return index;
	}
	else
	{
		LOG_ERROR("\"%s\" is not valid function address!", index.c_str());
		return index;
	}
}

void disassembler::print_script_name(const std::string& name)
{
#ifdef DEV_DEBUG
	printf("// IW6 PC GSCASM\n");
	printf("// Disassembled by https://github.com/xensik/gsc-tool\n\n");
#else
	output_->write_cpp_string("// IW6 PC GSCASM\n");
	output_->write_cpp_string("// Disassembled by https://github.com/xensik/gsc-tool\n");
#endif
}

void disassembler::print_opcodes(std::uint32_t index, std::uint32_t size)
{
	if (ida_output_)
	{
#ifdef DEV_DEBUG
		printf(utils::string::va("%04X\t", index).c_str());
		printf(utils::string::va("%-20s \t\t", script_->get_bytes_print(index, size).data()).c_str());
#else
		output_->write_cpp_string(utils::string::va("%04X\t", index));
		output_->write_cpp_string(utils::string::va("%-20s \t\t", script_->get_bytes_print(index, size).data()));
#endif
	}
	else
	{
#ifdef DEV_DEBUG
		printf("\t\t");
#else
		output_->write_cpp_string("\t\t");
#endif
	}
}

void disassembler::print_function(std::shared_ptr<function> func)
{
#ifdef DEV_DEBUG
	printf("\n");
#else
	output_->write_cpp_string("\n");
#endif
	if (ida_output_)
	{
#ifdef DEV_DEBUG
		printf(utils::string::va("\t%-20s", "", func->name.data()).c_str());
#else
		output_->write_cpp_string(utils::string::va("\t%-20s", ""));
#endif
	}

#ifdef DEV_DEBUG
		printf(utils::string::va("%s\n", func->name.data()).c_str());
#else
		output_->write_cpp_string(utils::string::va("%s\n", func->name.data()));
#endif
}

void disassembler::print_instruction(std::shared_ptr<instruction> inst)
{
	switch (opcode(inst->opcode))
	{
	case opcode::OP_endswitch:
		this->print_opcodes(inst->index, 3);
#ifdef DEV_DEBUG
		printf(utils::string::va("%s", opcode_name(opcode(inst->opcode)).data()).c_str());
		printf(utils::string::va(" %s\n", inst->data[0].data()).c_str());
#else
		output_->write_cpp_string(utils::string::va("%s", resolver::opcode_name(opcode(inst->opcode)).data()));
		output_->write_cpp_string(utils::string::va(" %s\n", inst->data[0].data()));
#endif
		{
			std::uint32_t totalcase = std::stoul(inst->data[0]);
			auto index = 0;
			for (auto casenum = 0; casenum < totalcase; casenum++)
			{
				this->print_opcodes(inst->index, 7);
				if (inst->data[1 + index] == "case")
				{
#ifdef DEV_DEBUG
					printf(utils::string::va("%s %s %s", inst->data[1 + index].data(), inst->data[1 + index + 1].data(), inst->data[1 + index + 2].data()).c_str());
#else
					output_->write_cpp_string(utils::string::va("%s %s %s", inst->data[1 + index].data(), inst->data[1 + index + 1].data(), inst->data[1 + index + 2].data()));
#endif
					index += 3;
				}
				else if (inst->data[1 + index] == "default")
				{
#ifdef DEV_DEBUG
					printf(utils::string::va("%s %s", inst->data[1 + index].data(), inst->data[1 + index + 1].data()).c_str());
#else
					output_->write_cpp_string(utils::string::va("%s %s", inst->data[1 + index].data(), inst->data[1 + index + 1].data()));
#endif
					index += 2;
				}
				if (casenum != totalcase - 1)
				{
#ifdef DEV_DEBUG
					printf("\n");
#else
					output_->write_cpp_string("\n");
#endif
				}
			}
		}
		break;
	default:
		this->print_opcodes(inst->index, inst->size);
#ifdef DEV_DEBUG
		printf(utils::string::va("%s", opcode_name(opcode(inst->opcode)).data()).c_str());
#else
		output_->write_cpp_string(utils::string::va("%s", resolver::opcode_name(opcode(inst->opcode)).data()));
#endif
		for (auto& d : inst->data)
		{
#ifdef DEV_DEBUG
			printf(utils::string::va(" %s", d.data()).c_str());
#else
			output_->write_cpp_string(utils::string::va(" %s", d.data()));
#endif
		}
		break;
	}

#ifdef DEV_DEBUG
	printf("\n");
#else
	output_->write_cpp_string("\n");
#endif
}

void disassembler::print_label(const std::string& label)
{
	if (ida_output_)
	{
		output_->write_cpp_string(utils::string::va("\n\t%-20s ", ""));
	}

	output_->write_cpp_string(utils::string::va("\t%s\n", label.data()));
}

} // namespace IW6
